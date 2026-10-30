#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <commdlg.h>

#include <glad/glad.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#pragma comment(lib, "comdlg32.lib")

struct HdrPreview {
    GLuint tex = 0;
    int w = 0;
    int h = 0;
};

inline void hdr_preview_destroy(HdrPreview& p) {
    if (p.tex != 0) {
        glDeleteTextures(1, &p.tex);
        p.tex = 0;
    }
    p.w = 0;
    p.h = 0;
}

inline bool open_hdr_file_dialog(std::wstring& out_path) {
    std::wstring buf(1024, L'\0');
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = L"HDR Image (*.hdr)\0*.hdr\0All Files\0*.*\0";
    ofn.lpstrFile = buf.data();
    ofn.nMaxFile = static_cast<DWORD>(buf.size());
    ofn.lpstrTitle = L"Select HDR";
    ofn.lpstrDefExt = L"hdr";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
    if (!GetOpenFileNameW(&ofn))
        return false;
    out_path = buf.c_str();
    return !out_path.empty();
}

struct HdrCursor {
    const unsigned char* p = nullptr;
    const unsigned char* end = nullptr;

    bool remain(size_t n) const {
        return p != nullptr && end != nullptr && size_t(end - p) >= n;
    }

    bool read(void* dst, size_t n) {
        if (!remain(n))
            return false;
        std::memcpy(dst, p, n);
        p += n;
        return true;
    }

    bool line(std::string& s) {
        s.clear();
        if (p >= end)
            return false;
        while (p < end) {
            const unsigned char c = *p++;
            if (c == '\n')
                return true;
            if (c != '\r')
                s.push_back(static_cast<char>(c));
            if (s.size() > 2048)
                return false;
        }
        return !s.empty();
    }
};

inline bool hdr_decode_scanline(HdrCursor& cur, int width, unsigned char* rgbe) {
    if (width <= 0)
        return false;

    unsigned char prefix[4];
    if (!cur.read(prefix, 4))
        return false;

    const bool rle = width >= 8 && width <= 0x7fff
        && prefix[0] == 2 && prefix[1] == 2 && (prefix[2] & 0x80) == 0;
    if (!rle) {
        std::memcpy(rgbe, prefix, 4);
        if (width == 1)
            return true;
        return cur.read(rgbe + 4, size_t(width - 1) * 4);
    }

    const int scan_w = (int(prefix[2]) << 8) | int(prefix[3]);
    if (scan_w != width)
        return false;

    for (int ch = 0; ch < 4; ++ch) {
        int x = 0;
        while (x < width) {
            unsigned char code = 0;
            if (!cur.read(&code, 1))
                return false;
            if (code > 128) {
                const int count = int(code) - 128;
                unsigned char val = 0;
                if (count <= 0 || x + count > width || !cur.read(&val, 1))
                    return false;
                for (int i = 0; i < count; ++i)
                    rgbe[(x++) * 4 + ch] = val;
            } else {
                const int count = int(code);
                if (count <= 0 || x + count > width)
                    return false;
                for (int i = 0; i < count; ++i) {
                    unsigned char val = 0;
                    if (!cur.read(&val, 1))
                        return false;
                    rgbe[(x++) * 4 + ch] = val;
                }
            }
        }
    }
    return true;
}

inline void hdr_rgbe_to_rgb(const unsigned char* rgbe, float* rgb) {
    if (rgbe[3] == 0) {
        rgb[0] = rgb[1] = rgb[2] = 0.f;
        return;
    }
    const float f = std::ldexp(1.f, int(rgbe[3]) - (128 + 8));
    rgb[0] = float(rgbe[0]) * f;
    rgb[1] = float(rgbe[1]) * f;
    rgb[2] = float(rgbe[2]) * f;
}

inline unsigned char hdr_tonemap_u8(float v) {
    v = v / (1.f + v);
    v = std::pow(std::max(v, 0.f), 1.f / 2.2f);
    return static_cast<unsigned char>(std::clamp(int(v * 255.f + 0.5f), 0, 255));
}

inline std::string hdr_preview_load(HdrPreview& dest, const std::wstring& path) {
    if (path.empty())
        return "\xE8\xB7\xAF\xE5\xBE\x84\xE4\xB8\xBA\xE7\xA9\xBA";

    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return "\xE6\x97\xA0\xE6\xB3\x95\xE6\x89\x93\xE5\xBC\x80\xE6\x96\x87\xE4\xBB\xB6";

    LARGE_INTEGER sz{};
    if (!GetFileSizeEx(file, &sz) || sz.QuadPart <= 0 || sz.QuadPart > 256ll * 1024 * 1024) {
        CloseHandle(file);
        return "HDR \xE6\x96\x87\xE4\xBB\xB6\xE8\xBF\x87\xE5\xA4\xA7\xE6\x88\x96\xE6\x97\xA0\xE6\x95\x88";
    }

    std::vector<unsigned char> bytes(static_cast<size_t>(sz.QuadPart));
    DWORD read = 0;
    const BOOL ok = ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr);
    CloseHandle(file);
    if (!ok || read != bytes.size())
        return "\xE8\xAF\xBB\xE5\x8F\x96 HDR \xE5\xA4\xB1\xE8\xB4\xA5";

    HdrCursor cur{ bytes.data(), bytes.data() + bytes.size() };
    std::string line;
    if (!cur.line(line) || (line.rfind("#?RADIANCE", 0) != 0 && line.rfind("#?RGBE", 0) != 0))
        return "\xE4\xB8\x8D\xE6\x98\xAF Radiance .hdr";

    while (cur.line(line) && !line.empty()) {
    }

    if (!cur.line(line))
        return "HDR \xE7\xBC\xBA\xE5\xB0\x91\xE5\x88\x86\xE8\xBE\xA8\xE7\x8E\x87\xE8\xA1\x8C";

    int height = 0;
    int width = 0;
    int y_sign = -1;
    if (std::sscanf(line.c_str(), "-Y %d +X %d", &height, &width) == 2) {
        y_sign = -1;
    } else if (std::sscanf(line.c_str(), "+Y %d +X %d", &height, &width) == 2) {
        y_sign = 1;
    } else {
        return "\xE4\xB8\x8D\xE6\x94\xAF\xE6\x8C\x81\xE7\x9A\x84 HDR \xE6\x96\xB9\xE5\x90\x91";
    }
    if (width <= 0 || height <= 0 || width > 16384 || height > 16384)
        return "HDR \xE5\xB0\xBA\xE5\xAF\xB8\xE6\x97\xA0\xE6\x95\x88";

    std::vector<float> rgb(size_t(width) * size_t(height) * 3);
    std::vector<unsigned char> scan(size_t(width) * 4);
    for (int y = 0; y < height; ++y) {
        if (!hdr_decode_scanline(cur, width, scan.data()))
            return "HDR \xE6\x89\xAB\xE6\x8F\x8F\xE8\xA1\x8C\xE8\xA7\xA3\xE7\xA0\x81\xE5\xA4\xB1\xE8\xB4\xA5";
        const int dst_y = (y_sign < 0) ? y : (height - 1 - y);
        float* row = rgb.data() + size_t(dst_y) * size_t(width) * 3;
        for (int x = 0; x < width; ++x)
            hdr_rgbe_to_rgb(scan.data() + x * 4, row + x * 3);
    }

    const int max_side = 512;
    const float scale = std::min(1.f, float(max_side) / float(std::max(width, height)));
    const int pw = std::max(1, int(float(width) * scale));
    const int ph = std::max(1, int(float(height) * scale));
    std::vector<unsigned char> rgba(size_t(pw) * size_t(ph) * 4);
    for (int py = 0; py < ph; ++py) {
        const int sy = std::min(height - 1, py * height / ph);
        for (int px = 0; px < pw; ++px) {
            const int sx = std::min(width - 1, px * width / pw);
            const float* s = rgb.data() + (size_t(sy) * size_t(width) + size_t(sx)) * 3;
            unsigned char* d = rgba.data() + (size_t(py) * size_t(pw) + size_t(px)) * 4;
            d[0] = hdr_tonemap_u8(s[0]);
            d[1] = hdr_tonemap_u8(s[1]);
            d[2] = hdr_tonemap_u8(s[2]);
            d[3] = 255;
        }
    }

    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    if (tex == 0)
        return "\xE5\x88\x9B\xE5\xBB\xBA\xE9\xA2\x84\xE8\xA7\x88\xE7\xBA\xB9\xE7\x90\x86\xE5\xA4\xB1\xE8\xB4\xA5";

    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage2D(tex, 1, GL_RGBA8, pw, ph);
    glTextureSubImage2D(tex, 0, 0, 0, pw, ph, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

    hdr_preview_destroy(dest);
    dest.tex = tex;
    dest.w = pw;
    dest.h = ph;
    return {};
}
