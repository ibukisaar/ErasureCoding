#include "GF.h"
#include <bitset>
#include <xmmintrin.h>
#include <memory.h>

#define EXPORT __declspec(dllexport)

template<size_t I>
static bool check_indexes(int indexCount, const int(*indexes)[2]) noexcept {
    std::bitset<256> bitmap;
    for (int i = 0; i < indexCount; i++) {
        if (bitmap[indexes[i][I]]) return false;
        bitmap[indexes[i][I]] = true;
    }
    return true;
}

// 矩阵求逆
static void mat_inverse(GF* mat, int stride) noexcept {
    GF invMat[stride * stride];
    for (int i = 0; i < stride; i++) {
        invMat[i * stride + i] = 1;
    }

    for (int i = 0; i < stride; i++) {
        for (int r = i + 1; r < stride; r++) {
            GF scale = mat[r * stride + i] / mat[i * stride + i];
            if (scale == 0) continue;
            mat[r * stride + i] = 0;
            for (int c = i + 1; c < stride; c++) {
                mat[r * stride + c] += mat[i * stride + c] * scale;
            }
            for (int c = 0; c < stride; c++) {
                invMat[r * stride + c] += invMat[i * stride + c] * scale;
            }
        }
    }

    for (int i = stride - 1; i >= 0; i--) {
        for (int r = i - 1; r >= 0; r--) {
            GF scale = mat[r * stride + i] / mat[i * stride + i];
            if (scale == 0) continue;
            for (int c = i - 1; c >= 0; c--) {
                mat[r * stride + c] += mat[i * stride + c] * scale;
            }
            for (int c = stride - 1; c >= 0; c--) {
                invMat[r * stride + c] += invMat[i * stride + c] * scale;
            }
        }
    }

    for (int i = 0; i < stride; i++) {
        GF scale = mat[i * stride + i].inverse();
        if (scale == 1) continue;
        for (int j = 0; j < stride; j++) invMat[i * stride + j] *= scale;
    }

    memcpy(mat, invMat, stride * stride * sizeof(GF));
}

extern "C" {
    EXPORT int ec_encode(const u8* __restrict data, int dataBytes, u8* __restrict erasureCode, int erasureCodeBytes, int dataStride, int erasureCodeStride) {
        if (dataStride <= 0 || dataStride >= 255) return 1;
        if (erasureCodeStride <= 0 || erasureCodeStride >= 255) return 2;
        if (dataBytes % dataStride != 0) return 3;
        if (erasureCodeBytes % erasureCodeStride != 0) return 4;

        int dataRows = dataBytes / dataStride;
        int ecRows = erasureCodeBytes / erasureCodeStride;
        if (dataRows != ecRows) return 5;

        GF const* multipliers[dataStride * erasureCodeStride];
        for (int r = 0, i = 0; r < erasureCodeStride; r++) {
            GF a = GF::from_exp(r);
            for (int c = 0; c < dataStride; c++, i++) {
                multipliers[i] = a.pow(c).multiplier();
                for (int offs = 0; offs < 256; offs += 64) {
                    _mm_prefetch(reinterpret_cast<const char*>(multipliers[i] + offs), _MM_HINT_NTA);
                }
            }
        }

        for (int r = 0, dataIndex = 0, ecIndex = 0; r < dataRows; r++, dataIndex += dataStride) {
            GF const* const* multiplier = multipliers;
            for (int ecSubIndex = 0; ecSubIndex < erasureCodeStride; ecSubIndex++, multiplier += dataStride) {
                GF s = 0;
                for (int i = 0; i < dataStride; i++) s += multiplier[i][data[dataIndex + i]];
                erasureCode[ecIndex++] = s;
            }
        }

        return 0;
    }

    EXPORT int ec_decode(u8* __restrict dataWithEC, int dataWithECBytes, int stride, const int(*__restrict indexes)[2], int indexCount) {
        if (stride <= 0 || stride >= 255) return 1;
        if (indexCount <= 0 || indexCount >= 255) return 2;
        if (dataWithECBytes % stride != 0) return 3;

        for (int i = 0; i < indexCount; i++) {
            if (indexes[i][0] < 0 || indexes[i][0] >= stride) return 4;
            if (indexes[i][1] < 0 || indexes[i][1] >= 254) return 4;
        }

        if (!check_indexes<0>(indexCount, indexes) || !check_indexes<1>(indexCount, indexes)) return 4;

        GF mat[stride * stride];
        for (int i = 0; i < stride; i++) {
            mat[i * stride + i] = 1;
        }

        for (int i = 0; i < indexCount; i++) {
            GF* row = mat + indexes[i][0] * stride;
            GF a = GF::from_exp(indexes[i][1]);
            for (int c = 0; c < stride; c++) {
                row[c] = a.pow(c);
            }
        }

        mat_inverse(mat, stride);

        GF const* multipliers[stride * indexCount];
        for (int r = 0; r < indexCount; r++) {
            for (int c = 0; c < stride; c++) {
                multipliers[r * stride + c] = mat[indexes[r][0] * stride + c].multiplier();
                for (int offs = 0; offs < 256; offs += 64) {
                    _mm_prefetch(reinterpret_cast<const char*>(multipliers[r * stride + c] + offs), _MM_HINT_NTA);
                }
            }
        }

        GF tmpResult[indexCount];
        int rows = dataWithECBytes / stride;
        for (int r = 0, dataIndex = 0; r < rows; r++, dataIndex += stride) {
            GF const* const* multiplier = multipliers;
            for (int i = 0; i < indexCount; i++, multiplier += stride) {
                GF s = 0;
                for (int c = 0; c < stride; c++) s += multiplier[c][dataWithEC[dataIndex + c]];
                tmpResult[i] = s;
            }
            for (int i = 0; i < indexCount; i++) {
                dataWithEC[dataIndex + indexes[i][0]] = tmpResult[i];
            }
        }

        return 0;
    }
}