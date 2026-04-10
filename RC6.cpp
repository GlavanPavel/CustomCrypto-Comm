#include "header.h"

const int W = 32;
const int R = 20;
const int NrS = 2 * R + 4;

const uint32_t P32 = 0xB7E15163;
const uint32_t Q32 = 0x9E3779B9;

uint32_t rotl(uint32_t x, uint32_t n)
{
    n &= (W - 1);
    return (x << n) | (x >> (W - n));
}

uint32_t rotr(uint32_t x, uint32_t n)
{
    n &= (W - 1);
    return (x >> n) | (x << (W - n));
}

vector<uint32_t> keySchedule(vector<uint8_t> &key)
{
    int b = static_cast<int>(key.size());
    int c = (b == 0) ? 1 : (b + 3) / 4;

    vector<uint32_t> L(c, 0);
    for (int i = b - 1; i >= 0; i--)
        L[i / 4] = (L[i / 4] << 8) | key[i];

    vector<uint32_t> S(NrS);
    S[0] = P32;

    for (int i = 1; i < NrS; i++)
        S[i] = S[i - 1] + Q32;

    uint32_t A = 0, B = 0;
    int i = 0, j = 0;

    int v = 3 * max(c, NrS);
    for (int s = 0; s < v; s++)
    {
        S[i] = rotl(S[i] + A + B, 3);
        A = S[i];
        L[j] = rotl(L[j] + A + B, (A + B) & (W - 1));
        B = L[j];
        i = (i + 1) % NrS;
        j = (j + 1) % c;
    }

    return S;
}

void encrypt_block(uint32_t registers[4], const vector<uint32_t> &S)
{
    uint32_t A = registers[0];
    uint32_t B = registers[1];
    uint32_t C = registers[2];
    uint32_t D = registers[3];

    B = B + S[0];
    D = D + S[1];

    for (int i = 1; i <= R; i++)
    {
        uint32_t t = rotl(B * (2 * B + 1), 5);
        uint32_t u = rotl(D * (2 * D + 1), 5);

        A = rotl(A ^ t, u) + S[2 * i];
        C = rotl(C ^ u, t) + S[2 * i + 1];

        uint32_t temp = A;
        A = B;
        B = C;
        C = D;
        D = temp;
    }

    A = A + S[2 * R + 2];
    C = C + S[2 * R + 3];

    registers[0] = A;
    registers[1] = B;
    registers[2] = C;
    registers[3] = D;
}

void decrypt_block(uint32_t registers[4], const vector<uint32_t> &S)
{
    uint32_t A = registers[0];
    uint32_t B = registers[1];
    uint32_t C = registers[2];
    uint32_t D = registers[3];

    C = C - S[2 * R + 3];
    A = A - S[2 * R + 2];

    for (int i = R; i >= 1; i--)
    {
        uint32_t temp = D;
        D = C;
        C = B;
        B = A;
        A = temp;

        uint32_t u = rotl(D * (2 * D + 1), 5);
        uint32_t t = rotl(B * (2 * B + 1), 5);

        C = rotr(C - S[2 * i + 1], t) ^ u;
        A = rotr(A - S[2 * i], u) ^ t;
    }

    D = D - S[1];
    B = B - S[0];

    registers[0] = A;
    registers[1] = B;
    registers[2] = C;
    registers[3] = D;
}

void print_block(uint32_t b[4])
{
    for (int i = 0; i < 4; i++)
        cout << hex << setw(8) << setfill('0') << b[i] << " ";
    cout << dec << endl;
}


// int main()
// {
//     test_vectori_oficiali();

//     vector<uint8_t> userKey = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16};
//     vector<uint32_t> S = keySchedule(userKey);

//     uint32_t myData[4] = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD};

//     cout << "Original:  ";
//     print_block(myData);

//     encrypt_block(myData, S);
//     cout << "Criptat:   ";
//     print_block(myData);

//     decrypt_block(myData, S);
//     cout << "Decriptat: ";
//     print_block(myData);

//     test_simetrie(S);
//     test_avalansa(S);
//     test_performanta(S);

//     return 0;
// }