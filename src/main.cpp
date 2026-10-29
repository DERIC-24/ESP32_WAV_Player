
#include <Arduino.h>
#include <SPIFFS.h>
#include <FS.h>
#include <cstring>

// Hardware configuration
constexpr uint8_t DAC_PIN = 25;
constexpr uint32_t TARGET_SAMPLE_RATE = 22050;

File audioFile;
uint32_t dataRemaining = 0;
uint32_t nextSampleTime = 0;
bool playbackActive = false;

// Read little-endian values safely.
bool readBytes(File &file, uint8_t *buffer, size_t count) {
  return file.read(buffer, count) == (int)count;
}

bool read16(File &file, uint16_t &value) {
  uint8_t b[2];
  if (!readBytes(file, b, 2)) return false;
  value = (uint16_t)b[0] | ((uint16_t)b[1] << 8);
  return true;
}

bool read32(File &file, uint32_t &value) {
  uint8_t b[4];
  if (!readBytes(file, b, 4)) return false;
  value = (uint32_t)b[0]
        | ((uint32_t)b[1] << 8)
        | ((uint32_t)b[2] << 16)
        | ((uint32_t)b[3] << 24);
  return true;
}

bool readID(File &file, char id[5]) {
  id[4] = '\0';
  return readBytes(file, (uint8_t *)id, 4);
}

bool openWav() {
  audioFile = SPIFFS.open("/audio.wav", FILE_READ);

  if (!audioFile) {
    Serial.println("ERROR: /audio.wav not found");
    return false;
  }

  char id[5];

  uint32_t riffSize;
  if (!readID(audioFile, id) || strcmp(id, "RIFF") != 0 ||
      !read32(audioFile, riffSize) ||
      !readID(audioFile, id) || strcmp(id, "WAVE") != 0) {
    Serial.println("ERROR: Invalid WAV/RIFF file");
    audioFile.close();
    return false;
  }

  bool fmtFound = false;
  bool dataFound = false;

  uint16_t format = 0;
  uint16_t channels = 0;
  uint16_t bits = 0;
  uint16_t blockAlign = 0;
  uint32_t sampleRate = 0;
  uint32_t dataSize = 0;
  uint32_t dataPosition = 0;

  while (audioFile.available()) {
    if (!readID(audioFile, id)) break;

    uint32_t chunkSize;
    if (!read32(audioFile, chunkSize)) break;

    uint32_t chunkStart = audioFile.position();

    // Reject malformed chunk lengths.
    if (chunkSize > audioFile.size() - chunkStart) break;

    if (strcmp(id, "fmt ") == 0) {
      if (chunkSize < 16) break;

      uint32_t byteRate;
      if (!read16(audioFile, format) ||
          !read16(audioFile, channels) ||
          !read32(audioFile, sampleRate) ||
          !read32(audioFile, byteRate) ||
          !read16(audioFile, blockAlign) ||
          !read16(audioFile, bits)) {
        break;
      }

      fmtFound = true;
    } else if (strcmp(id, "data") == 0) {
      dataPosition = chunkStart;
      dataSize = chunkSize;
      dataFound = true;
    }

    // WAV chunks are padded to an even number of bytes.
    uint32_t nextChunk = chunkStart + chunkSize + (chunkSize & 1);
    if (nextChunk > audioFile.size()) break;
    if (!audioFile.seek(nextChunk)) break;

    if (fmtFound && dataFound) break;
  }

  if (!fmtFound || !dataFound ||
      format != 1 || channels != 1 || bits != 8 ||
      sampleRate != TARGET_SAMPLE_RATE || blockAlign != 1) {
    Serial.println(
      "ERROR: WAV must be PCM, mono, 8-bit, 22050 Hz"
    );
    audioFile.close();
    return false;
  }

  if (!audioFile.seek(dataPosition)) {
    Serial.println("ERROR: Cannot seek to audio data");
    audioFile.close();
    return false;
  }

  dataRemaining = dataSize;
  Serial.printf(
    "WAV ready: %lu Hz, %lu audio bytes\n",
    (unsigned long)sampleRate,
    (unsigned long)dataSize
  );

  return true;
}

void stopPlayback() {
  playbackActive = false;

  if (audioFile) audioFile.close();

  // Midpoint DAC value represents silence for unsigned PCM.
  dacWrite(DAC_PIN, 128);
  Serial.println("Playback finished");
}

void setup() {
  Serial.begin(115200);
  dacWrite(DAC_PIN, 128);

  Serial.println("\nESP32 WAV Audio Player");

  if (!SPIFFS.begin(false)) {
    Serial.println("ERROR: SPIFFS mount failed");
    Serial.println("Build and upload the filesystem image.");
    return;
  }

  Serial.printf(
    "SPIFFS total: %u, used: %u bytes\n",
    (unsigned)SPIFFS.totalBytes(),
    (unsigned)SPIFFS.usedBytes()
  );

  if (openWav()) {
    playbackActive = true;
    nextSampleTime = micros();
    Serial.println("Playback starting...");
  }
}

void loop() {
  if (!playbackActive) {
    delay(10);
    return;
  }

  // Signed subtraction handles micros() rollover.
  if ((int32_t)(micros() - nextSampleTime) < 0) {
    return;
  }

  if (dataRemaining == 0) {
    stopPlayback();
    return;
  }

  int sample = audioFile.read();

  if (sample < 0) {
    Serial.println("ERROR: Unexpected end of WAV file");
    stopPlayback();
    return;
  }

  dacWrite(DAC_PIN, (uint8_t)sample);
  dataRemaining--;

  // Schedule at the requested sample rate.
  nextSampleTime += 1000000UL / TARGET_SAMPLE_RATE;

  // Avoid a long burst if execution falls behind.
  if ((int32_t)(micros() - nextSampleTime) >
      (int32_t)(1000000UL / TARGET_SAMPLE_RATE)) {
    nextSampleTime = micros();
  }
}
