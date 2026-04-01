#ifndef SIMPLE_WAV_HPP
#define SIMPLE_WAV_HPP

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>

class WavWriter {
public:
    enum Result {
        OK = 0,
        ERR_INVALID_ARG = -1,
        ERR_IO = -2,
        ERR_BAD_FORMAT = -3,
        ERR_UNSUPPORTED = -4,
        ERR_SEEK = -5
    };

public:
    WavWriter()
        : fp_(NULL),
          channels_(0),
          sample_rate_(0),
          bits_per_sample_(0),
          bytes_per_sample_(0),
          data_bytes_written_(0),
          riff_size_offset_(0),
          data_size_offset_(0),
          owns_file_(0) {
    }

    ~WavWriter() {
        close();
    }

    WavWriter(const WavWriter&) = delete;
    WavWriter& operator=(const WavWriter&) = delete;

    Result init(const char* path, uint16_t channels, uint32_t sample_rate, uint16_t bits_per_sample) {
        FILE* fp;

        if (!path) return ERR_INVALID_ARG;
        if (channels == 0) return ERR_INVALID_ARG;
        if (!(bits_per_sample == 8 || bits_per_sample == 16 || bits_per_sample == 32)) return ERR_UNSUPPORTED;

        close();

        fp = std::fopen(path, "wb");
        if (!fp) return ERR_IO;

        fp_ = fp;
        owns_file_ = 1;
        channels_ = channels;
        sample_rate_ = sample_rate;
        bits_per_sample_ = bits_per_sample;
        bytes_per_sample_ = bits_per_sample / 8;
        data_bytes_written_ = 0;

        return write_header();
    }

    Result write_frame(const void* src, size_t numSample) {
        size_t bytes;

        if (!fp_ || !src) return ERR_INVALID_ARG;
        bytes = numSample * bytes_per_sample_;

        if (std::fwrite(src, 1, bytes, fp_) != bytes) {
            return ERR_IO;
        }

        data_bytes_written_ += (uint32_t)bytes;
        return OK;
    }

    void close() {
        if (!fp_) return;

        finalize();

        if (owns_file_) {
            std::fclose(fp_);
        }

        fp_ = NULL;
        channels_ = 0;
        sample_rate_ = 0;
        bits_per_sample_ = 0;
        bytes_per_sample_ = 0;
        data_bytes_written_ = 0;
        riff_size_offset_ = 0;
        data_size_offset_ = 0;
        owns_file_ = 0;
    }

    uint16_t channels() const { return channels_; }
    uint32_t sample_rate() const { return sample_rate_; }
    uint16_t bits_per_sample() const { return bits_per_sample_; }

private:
    static int write_u16le(FILE* fp, uint16_t v) {
        uint8_t b[2];
        b[0] = (uint8_t)(v & 0xFF);
        b[1] = (uint8_t)((v >> 8) & 0xFF);
        return std::fwrite(b, 1, 2, fp) == 2;
    }

    static int write_u32le(FILE* fp, uint32_t v) {
        uint8_t b[4];
        b[0] = (uint8_t)(v & 0xFF);
        b[1] = (uint8_t)((v >> 8) & 0xFF);
        b[2] = (uint8_t)((v >> 16) & 0xFF);
        b[3] = (uint8_t)((v >> 24) & 0xFF);
        return std::fwrite(b, 1, 4, fp) == 4;
    }

    static int ftell_u32(FILE* fp, uint32_t* out) {
        long pos = std::ftell(fp);
        if (pos < 0) return 0;
        *out = (uint32_t)pos;
        return 1;
    }

    Result write_header() {
        uint16_t block_align;
        uint32_t byte_rate;

        block_align = (uint16_t)(channels_ * bytes_per_sample_);
        byte_rate = sample_rate_ * block_align;

        if (std::fwrite("RIFF", 1, 4, fp_) != 4) return ERR_IO;
        if (!ftell_u32(fp_, &riff_size_offset_)) return ERR_SEEK;
        if (!write_u32le(fp_, 0)) return ERR_IO;
        if (std::fwrite("WAVE", 1, 4, fp_) != 4) return ERR_IO;

        if (std::fwrite("fmt ", 1, 4, fp_) != 4) return ERR_IO;
        if (!write_u32le(fp_, 16)) return ERR_IO;
        if (!write_u16le(fp_, 1)) return ERR_IO;
        if (!write_u16le(fp_, channels_)) return ERR_IO;
        if (!write_u32le(fp_, sample_rate_)) return ERR_IO;
        if (!write_u32le(fp_, byte_rate)) return ERR_IO;
        if (!write_u16le(fp_, block_align)) return ERR_IO;
        if (!write_u16le(fp_, bits_per_sample_)) return ERR_IO;

        if (std::fwrite("data", 1, 4, fp_) != 4) return ERR_IO;
        if (!ftell_u32(fp_, &data_size_offset_)) return ERR_SEEK;
        if (!write_u32le(fp_, 0)) return ERR_IO;

        return OK;
    }

    Result finalize() {
        uint32_t file_end;
        uint32_t riff_size;
        long cur;

        if (!fp_) return ERR_INVALID_ARG;

        if (data_bytes_written_ & 1u) {
            uint8_t zero = 0;
            if (std::fwrite(&zero, 1, 1, fp_) != 1) return ERR_IO;
        }

        cur = std::ftell(fp_);
        if (cur < 0) return ERR_SEEK;
        file_end = (uint32_t)cur;
        riff_size = file_end - 8;

        if (std::fseek(fp_, (long)riff_size_offset_, SEEK_SET) != 0) return ERR_SEEK;
        if (!write_u32le(fp_, riff_size)) return ERR_IO;

        if (std::fseek(fp_, (long)data_size_offset_, SEEK_SET) != 0) return ERR_SEEK;
        if (!write_u32le(fp_, data_bytes_written_)) return ERR_IO;

        if (std::fseek(fp_, (long)file_end, SEEK_SET) != 0) return ERR_SEEK;
        if (std::fflush(fp_) != 0) return ERR_IO;

        return OK;
    }

private:
    FILE* fp_;
    uint16_t channels_;
    uint32_t sample_rate_;
    uint16_t bits_per_sample_;
    uint16_t bytes_per_sample_;

    uint32_t data_bytes_written_;
    uint32_t riff_size_offset_;
    uint32_t data_size_offset_;
    int owns_file_;
};

class WavReader {
public:
    enum Result {
        OK = 0,
        ERR_INVALID_ARG = -1,
        ERR_IO = -2,
        ERR_BAD_FORMAT = -3,
        ERR_UNSUPPORTED = -4,
        ERR_SEEK = -5
    };

public:
    WavReader()
        : fp_(NULL),
          channels_(0),
          sample_rate_(0),
          bits_per_sample_(0),
          bytes_per_sample_(0),
          data_offset_(0),
          data_size_(0),
          data_bytes_left_(0),
          owns_file_(0) {
    }

    ~WavReader() {
        close();
    }

    WavReader(const WavReader&) = delete;
    WavReader& operator=(const WavReader&) = delete;

    Result init(const char* path) {
        FILE* fp;
        if (!path) return ERR_INVALID_ARG;

        close();

        fp = std::fopen(path, "rb");
        if (!fp) return ERR_IO;

        fp_ = fp;
        owns_file_ = 1;
        return parse_header();
    }

    Result read_frame(void* dst, size_t numSample, size_t* numReadSample = NULL) {
        size_t bytes;
        size_t want;
        size_t got;

        if (numReadSample) *numReadSample = 0;
        if (!fp_ || !dst) return ERR_INVALID_ARG;

        bytes = numSample * bytes_per_sample_;
        want = bytes;
        if ((uint32_t)want > data_bytes_left_) {
            want = data_bytes_left_;
        }

        got = std::fread(dst, 1, want, fp_);
        data_bytes_left_ -= (uint32_t)got;

        if (numReadSample) {
            *numReadSample = got / bytes_per_sample_;
        }

        if (got != want && std::ferror(fp_)) {
            return ERR_IO;
        }

        return OK;
    }

    void close() {
        if (fp_ && owns_file_) {
            std::fclose(fp_);
        }

        fp_ = NULL;
        channels_ = 0;
        sample_rate_ = 0;
        bits_per_sample_ = 0;
        bytes_per_sample_ = 0;
        data_offset_ = 0;
        data_size_ = 0;
        data_bytes_left_ = 0;
        owns_file_ = 0;
    }

    uint16_t channels() const { return channels_; }
    uint32_t sample_rate() const { return sample_rate_; }
    uint16_t bits_per_sample() const { return bits_per_sample_; }

private:
    static int read_u16le(FILE* fp, uint16_t* out) {
        uint8_t b[2];
        if (std::fread(b, 1, 2, fp) != 2) return 0;
        *out = (uint16_t)(b[0] | ((uint16_t)b[1] << 8));
        return 1;
    }

    static int read_u32le(FILE* fp, uint32_t* out) {
        uint8_t b[4];
        if (std::fread(b, 1, 4, fp) != 4) return 0;
        *out = ((uint32_t)b[0]) |
               ((uint32_t)b[1] << 8) |
               ((uint32_t)b[2] << 16) |
               ((uint32_t)b[3] << 24);
        return 1;
    }

    static int ftell_u32(FILE* fp, uint32_t* out) {
        long pos = std::ftell(fp);
        if (pos < 0) return 0;
        *out = (uint32_t)pos;
        return 1;
    }

    static int id_eq(const char a[4], const char b[4]) {
        return a[0] == b[0] &&
               a[1] == b[1] &&
               a[2] == b[2] &&
               a[3] == b[3];
    }

    Result parse_header() {
        char riff[4];
        char wave[4];
        uint32_t riff_size;
        int found_fmt = 0;

        if (std::fread(riff, 1, 4, fp_) != 4) return ERR_IO;
        if (!read_u32le(fp_, &riff_size)) return ERR_IO;
        if (std::fread(wave, 1, 4, fp_) != 4) return ERR_IO;
        (void)riff_size;

        if (!id_eq(riff, "RIFF") || !id_eq(wave, "WAVE")) {
            return ERR_BAD_FORMAT;
        }

        for (;;) {
            char chunk_id[4];
            uint32_t chunk_size;
            uint32_t chunk_data_pos;

            if (std::fread(chunk_id, 1, 4, fp_) != 4) return ERR_BAD_FORMAT;
            if (!read_u32le(fp_, &chunk_size)) return ERR_BAD_FORMAT;
            if (!ftell_u32(fp_, &chunk_data_pos)) return ERR_SEEK;

            if (id_eq(chunk_id, "fmt ")) {
                uint16_t audio_format;
                uint16_t block_align;
                uint32_t byte_rate;

                if (chunk_size < 16) return ERR_BAD_FORMAT;

                if (!read_u16le(fp_, &audio_format)) return ERR_IO;
                if (!read_u16le(fp_, &channels_)) return ERR_IO;
                if (!read_u32le(fp_, &sample_rate_)) return ERR_IO;
                if (!read_u32le(fp_, &byte_rate)) return ERR_IO;
                if (!read_u16le(fp_, &block_align)) return ERR_IO;
                if (!read_u16le(fp_, &bits_per_sample_)) return ERR_IO;

                if (audio_format != 1) return ERR_UNSUPPORTED;
                if (!(bits_per_sample_ == 8 || bits_per_sample_ == 16 || bits_per_sample_ == 32)) return ERR_UNSUPPORTED;
                if (channels_ == 0) return ERR_BAD_FORMAT;

                bytes_per_sample_ = bits_per_sample_ / 8;

                if (block_align != channels_ * bytes_per_sample_) return ERR_BAD_FORMAT;
                if (byte_rate != sample_rate_ * block_align) return ERR_BAD_FORMAT;

                if (chunk_size > 16) {
                    if (std::fseek(fp_, (long)(chunk_size - 16), SEEK_CUR) != 0) return ERR_SEEK;
                }

                if (chunk_size & 1u) {
                    if (std::fseek(fp_, 1, SEEK_CUR) != 0) return ERR_SEEK;
                }

                found_fmt = 1;
            } else if (id_eq(chunk_id, "data")) {
                if (!found_fmt) return ERR_BAD_FORMAT;

                data_offset_ = chunk_data_pos;
                data_size_ = chunk_size;
                data_bytes_left_ = chunk_size;
                return OK;
            } else {
                uint32_t skip = chunk_size + (chunk_size & 1u);
                if (std::fseek(fp_, (long)skip, SEEK_CUR) != 0) return ERR_SEEK;
            }
        }
    }

private:
    FILE* fp_;
    uint16_t channels_;
    uint32_t sample_rate_;
    uint16_t bits_per_sample_;
    uint16_t bytes_per_sample_;

    uint32_t data_offset_;
    uint32_t data_size_;
    uint32_t data_bytes_left_;
    int owns_file_;
};

#endif /* SIMPLE_WAV_HPP */