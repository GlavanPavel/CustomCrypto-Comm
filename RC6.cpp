#include <iostream>
#include <vector>

using namespace std;

const int W = 32;
const int R = 20;
const int NrS = 2 * R + 3;

const uint32_t P32 = 0xB7E15163;
const uint32_t Q32 = 0x9E3779B9;

uint32_t rotl(uint32_t x, uint32_t n)
{
	n &= (W - 1);
	return (x << n) | (x >> (W - n));
}

vector<uint32_t> keySchedule(vector<uint8_t>& key)
{
	int b = static_cast<int>(key.size());
	int c = (b == 0) ? 1 : (b + 3) / 4;

	vector<uint32_t>L(c, 0);
	for (int i = b - 1; i >= 0; i--)
		L[i / 4] = (L[i / 4] << 8) | key[i];

	vector<uint32_t>S(NrS);
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