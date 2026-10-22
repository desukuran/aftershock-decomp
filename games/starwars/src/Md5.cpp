// MD5 message digest (RFC 1321).
#include "Md5.h"
#include <stdio.h>

static BYTE PADDING[64] = { 0x80 };

#define F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | ~(z)))

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define FF(a, b, c, d, x, s, ac) { (a) += F((b), (c), (d)) + (x) + (DWORD)(ac); (a) = ROTATE_LEFT((a), (s)) + (b); }
#define GG(a, b, c, d, x, s, ac) { (a) += G((b), (c), (d)) + (x) + (DWORD)(ac); (a) = ROTATE_LEFT((a), (s)) + (b); }
#define HH(a, b, c, d, x, s, ac) { (a) += H((b), (c), (d)) + (x) + (DWORD)(ac); (a) = ROTATE_LEFT((a), (s)) + (b); }
#define II(a, b, c, d, x, s, ac) { (a) += I((b), (c), (d)) + (x) + (DWORD)(ac); (a) = ROTATE_LEFT((a), (s)) + (b); }

// 0x00011920
MD5::MD5()
{
    memset(m_Buffer, 0, sizeof(m_Buffer));
    m_Count[0] = m_Count[1] = 0;
    m_State[0] = 0x67452301;
    m_State[1] = 0xEFCDAB89;
    m_State[2] = 0x98BADCFE;
    m_State[3] = 0x10325476;
}

// 0x00011030
void MD5::Transform(const BYTE block[64])
{
    DWORD a = m_State[0], b = m_State[1], c = m_State[2], d = m_State[3];
    DWORD x[16];
    for (int i = 0; i < 16; i++)
        x[i] = block[i * 4] | (block[i * 4 + 1] << 8) | (block[i * 4 + 2] << 16) | (block[i * 4 + 3] << 24);

    FF(a, b, c, d, x[ 0],  7, 0xd76aa478); FF(d, a, b, c, x[ 1], 12, 0xe8c7b756);
    FF(c, d, a, b, x[ 2], 17, 0x242070db); FF(b, c, d, a, x[ 3], 22, 0xc1bdceee);
    FF(a, b, c, d, x[ 4],  7, 0xf57c0faf); FF(d, a, b, c, x[ 5], 12, 0x4787c62a);
    FF(c, d, a, b, x[ 6], 17, 0xa8304613); FF(b, c, d, a, x[ 7], 22, 0xfd469501);
    FF(a, b, c, d, x[ 8],  7, 0x698098d8); FF(d, a, b, c, x[ 9], 12, 0x8b44f7af);
    FF(c, d, a, b, x[10], 17, 0xffff5bb1); FF(b, c, d, a, x[11], 22, 0x895cd7be);
    FF(a, b, c, d, x[12],  7, 0x6b901122); FF(d, a, b, c, x[13], 12, 0xfd987193);
    FF(c, d, a, b, x[14], 17, 0xa679438e); FF(b, c, d, a, x[15], 22, 0x49b40821);

    GG(a, b, c, d, x[ 1],  5, 0xf61e2562); GG(d, a, b, c, x[ 6],  9, 0xc040b340);
    GG(c, d, a, b, x[11], 14, 0x265e5a51); GG(b, c, d, a, x[ 0], 20, 0xe9b6c7aa);
    GG(a, b, c, d, x[ 5],  5, 0xd62f105d); GG(d, a, b, c, x[10],  9, 0x02441453);
    GG(c, d, a, b, x[15], 14, 0xd8a1e681); GG(b, c, d, a, x[ 4], 20, 0xe7d3fbc8);
    GG(a, b, c, d, x[ 9],  5, 0x21e1cde6); GG(d, a, b, c, x[14],  9, 0xc33707d6);
    GG(c, d, a, b, x[ 3], 14, 0xf4d50d87); GG(b, c, d, a, x[ 8], 20, 0x455a14ed);
    GG(a, b, c, d, x[13],  5, 0xa9e3e905); GG(d, a, b, c, x[ 2],  9, 0xfcefa3f8);
    GG(c, d, a, b, x[ 7], 14, 0x676f02d9); GG(b, c, d, a, x[12], 20, 0x8d2a4c8a);

    HH(a, b, c, d, x[ 5],  4, 0xfffa3942); HH(d, a, b, c, x[ 8], 11, 0x8771f681);
    HH(c, d, a, b, x[11], 16, 0x6d9d6122); HH(b, c, d, a, x[14], 23, 0xfde5380c);
    HH(a, b, c, d, x[ 1],  4, 0xa4beea44); HH(d, a, b, c, x[ 4], 11, 0x4bdecfa9);
    HH(c, d, a, b, x[ 7], 16, 0xf6bb4b60); HH(b, c, d, a, x[10], 23, 0xbebfbc70);
    HH(a, b, c, d, x[13],  4, 0x289b7ec6); HH(d, a, b, c, x[ 0], 11, 0xeaa127fa);
    HH(c, d, a, b, x[ 3], 16, 0xd4ef3085); HH(b, c, d, a, x[ 6], 23, 0x04881d05);
    HH(a, b, c, d, x[ 9],  4, 0xd9d4d039); HH(d, a, b, c, x[12], 11, 0xe6db99e5);
    HH(c, d, a, b, x[15], 16, 0x1fa27cf8); HH(b, c, d, a, x[ 2], 23, 0xc4ac5665);

    II(a, b, c, d, x[ 0],  6, 0xf4292244); II(d, a, b, c, x[ 7], 10, 0x432aff97);
    II(c, d, a, b, x[14], 15, 0xab9423a7); II(b, c, d, a, x[ 5], 21, 0xfc93a039);
    II(a, b, c, d, x[12],  6, 0x655b59c3); II(d, a, b, c, x[ 3], 10, 0x8f0ccc92);
    II(c, d, a, b, x[10], 15, 0xffeff47d); II(b, c, d, a, x[ 1], 21, 0x85845dd1);
    II(a, b, c, d, x[ 8],  6, 0x6fa87e4f); II(d, a, b, c, x[15], 10, 0xfe2ce6e0);
    II(c, d, a, b, x[ 6], 15, 0xa3014314); II(b, c, d, a, x[13], 21, 0x4e0811a1);
    II(a, b, c, d, x[ 4],  6, 0xf7537e82); II(d, a, b, c, x[11], 10, 0xbd3af235);
    II(c, d, a, b, x[ 2], 15, 0x2ad7d2bb); II(b, c, d, a, x[ 9], 21, 0xeb86d391);

    m_State[0] += a;
    m_State[1] += b;
    m_State[2] += c;
    m_State[3] += d;
}

// 0x00011960
void MD5::Update(const BYTE* pInput, unsigned int nLength)
{
    unsigned int index = (m_Count[0] >> 3) & 0x3F;
    if ((m_Count[0] += nLength << 3) < (nLength << 3))
        m_Count[1]++;
    m_Count[1] += nLength >> 29;

    unsigned int partLen = 64 - index;
    unsigned int i;
    if (nLength >= partLen)
    {
        memcpy(&m_Buffer[index], pInput, partLen);
        Transform(m_Buffer);
        for (i = partLen; i + 63 < nLength; i += 64)
            Transform(&pInput[i]);
        index = 0;
    }
    else
    {
        i = 0;
    }
    memcpy(&m_Buffer[index], &pInput[i], nLength - i);
}

// 0x00011FB0
std::string MD5::Final()
{
    BYTE bits[8];
    memcpy(bits, m_Count, 8);

    unsigned int index = (m_Count[0] >> 3) & 0x3F;
    unsigned int padLen = (index < 56) ? (56 - index) : (120 - index);
    Update(PADDING, padLen);
    Update(bits, 8);

    BYTE digest[16];
    memcpy(digest, m_State, 16);

    std::string strHex;
    for (int i = 0; i < 16; i++)
    {
        std::string strByte;
        if (digest[i] == 0)
        {
            strByte = "00";
        }
        else
        {
            char buf[256];
            if (digest[i] < 16)
                sprintf(buf, "0%x", digest[i]);
            else
                sprintf(buf, "%x", digest[i]);
            strByte = buf;
        }
        strHex += strByte;
    }
    return strHex;
}

// 0x00012310
std::string MD5String(const char* strText, unsigned int nLength)
{
    MD5 md5;
    md5.Update((const BYTE*)strText, nLength);
    return md5.Final();
}

// 0x00012210
// The digest of the rest of an open file, read 1 KB at a time.
std::string MD5File(FILE* f)
{
    MD5 md5;
    BYTE buffer[1024];
    int n = fread(buffer, 1, sizeof(buffer), f);
    while (n > 0)
    {
        md5.Update(buffer, n);
        n = fread(buffer, 1, sizeof(buffer), f);
    }
    return md5.Final();
}

// 0x000123C0
// The digest of a file (which must exist: fopen isn't checked).
std::string MD5FilePath(const char* strFile)
{
    FILE* f = fopen(strFile, "rb");
    std::string strDigest = MD5File(f);
    fclose(f);
    return strDigest;
}
