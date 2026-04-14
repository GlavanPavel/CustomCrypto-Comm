# include "header.h"

// dif de biti
int numara_diferente_biti(uint32_t block1[4], uint32_t block2[4])
{
    int diferente = 0;
    for (int i = 0; i < 4; i++)
    {
        diferente += bitset<32>(block1[i] ^ block2[i]).count();
    }
    return diferente;
}

//Tests for RC6, data block
void test_simetrie(const vector<uint32_t> &S)
{
    mt19937 gen(12345);
    uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);

    for (int i = 0; i < 10000; i++)
    {
        uint32_t original[4] = {dist(gen), dist(gen), dist(gen), dist(gen)};
        uint32_t de_lucru[4];

        for (int j = 0; j < 4; j++)
            de_lucru[j] = original[j];

        encrypt_block(de_lucru, S);
        decrypt_block(de_lucru, S);

        for (int j = 0; j < 4; j++)
        {
            assert(original[j] == de_lucru[j] && "error");
        }
    }
    cout << "test simertie ok\n"
         << endl;
}

void test_avalansa(const vector<uint32_t> &S)
{

    uint32_t original[4] = {0x11111111, 0x22222222, 0x33333333, 0x44444444};
    uint32_t block_modificat[4];
    for (int i = 0; i < 4; i++)
        block_modificat[i] = original[i];

    block_modificat[0] ^= 1;

    uint32_t criptat_original[4], criptat_modificat[4];
    for (int i = 0; i < 4; i++)
    {
        criptat_original[i] = original[i];
        criptat_modificat[i] = block_modificat[i];
    }

    encrypt_block(criptat_original, S);
    encrypt_block(criptat_modificat, S);

    int biti_schimbati = numara_diferente_biti(criptat_original, criptat_modificat);

    cout << "biti modificati la iesire din 128: " << biti_schimbati << endl;
    cout << "procentaj schimbare " << (biti_schimbati / 128.0) * 100 << "\n"
         << endl;
}
void test_performanta(const vector<uint32_t> &S)
{

    const int NUMAR_BLOCURI = 1000000;
    uint32_t buffer[4] = {0x01020304, 0x05060708, 0x090A0B0C, 0x0D0E0F00};

    auto start_time = chrono::high_resolution_clock::now();

    for (int i = 0; i < NUMAR_BLOCURI; i++)
    {
        encrypt_block(buffer, S);
    }

    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> diferenta = end_time - start_time;

    double megaBytes = (NUMAR_BLOCURI * 16.0) / (1024 * 1024);
    double viteza_MBps = megaBytes / diferenta.count();

    cout << "date procesate: " << megaBytes << endl;
    cout << "timp criptare: " << diferenta.count() << endl;
    cout << "viteza de criptare: " << viteza_MBps << endl;
}

void test_vectori_oficiali()
{
    vector<uint8_t> key0(16, 0);
    uint32_t pt0[4] = {0, 0, 0, 0};

    vector<uint32_t> S = keySchedule(key0);
    encrypt_block(pt0, S);

    cout << "Test Vector #1:" << endl;
    cout << "Rezultat obtinut: ";
    print_block(pt0);
    cout << "Rezultat asteptat: 36a5c38f 78f7b156 4edf29c1 1ea44898" << endl;

    cout << "---------------------------------------" << endl;

    vector<uint8_t> key2 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78};
    uint32_t pt2[4] = {0x35241302, 0x79685746, 0xbdac9b8a, 0xf1e0dfce};

    S = keySchedule(key2);
    encrypt_block(pt2, S);

    cout << "Test Vector #2:" << endl;
    cout << "Obtinut:  ";
    print_block(pt2);
    cout << "Asteptat: 2f194e52 23c61547 36f6511f 183fa47e" << endl;
}

// Tests key agreement Diffie-Hellman
void test_data_received()
{
    Party a;
    Party b(a.getP(), a.getG());
    b.create_shared_secret(a.sendPublicKey());
    a.create_shared_secret(b.sendPublicKey());
for(int i=0;i<2;i++)
{
    vector<uint32_t> myData = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD, 0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD};

    cout << "Data A wants to send :" << endl;
    print_data(myData);

    cout << "Data B receives: " << endl;
    vector<uint32_t> enc = a.encrypt_data(myData);
    print_data(enc);

    cout << "Data B decrypts" << endl;
    vector<uint32_t> dec = b.decrypt_data(enc);
    print_data(dec);
}
}
