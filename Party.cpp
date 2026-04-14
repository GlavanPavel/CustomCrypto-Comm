#include "header.h"

Integer calc(Integer base, Integer exp, Integer module)
{
    Integer result = 1;
    while (exp != 1)
    {
        if (exp % 2 == 1)
        {
            result = (result * base) % module;
            exp--;
        }
        exp = exp / 2;
        base = (base * base) % module;
    }
    result = (result * base) % module;
    return result;
}

Party::Party()
{
    AutoSeededRandomPool rnd;
    PrimeAndGenerator pg;
    pg.Generate(1, rnd, k, k - 1);
    p = pg.Prime();
    g = pg.Generator();
    cout << "p: " << p << endl
         << "g: " << g << endl;
    createKeys();
}

Party::Party(Integer P, Integer G)
{
    p = P;
    g = G;
    createKeys();
}

Integer Party::getP()
{
    return p;
}
Integer Party::getG()
{
    return g;
}
Integer Party::sendPublicKey()
{
    return public_key;
}

void Party::create_shared_secret(Integer key_recived)
{
    shared_secret = calc(key_recived, private_key, p);
    getKeyFromSecret();
}

vector<uint32_t> Party::encrypt_data(vector<uint32_t> data)
{
    while (data.size() % 4 != 0)
        data.push_back(0);
    
    AutoSeededRandomPool rnd;
    vector<uint32_t> iv(4);
    for(int i=0; i<4; i++)
    {
        iv[i] = (uint32_t)rnd.GenerateWord32();
    }
    data.insert(data.begin(),iv.begin(),iv.end());

    for (size_t i = 4; i < data.size(); i += 4)
    {
        uint32_t block[4];
        for(int j=0; j<4; j++)
        {
            block[j] = data[i+j] ^ data[i+j-4];
        }
        encrypt_block(block, S);
        for (int j = 0; j < 4; j++)
        {
            data[i + j] = block[j];
        }
    }
    return data;
}

vector<uint32_t> Party::decrypt_data(vector<uint32_t> data)
{

    vector<uint32_t> prev = {data[0], data[1], data[2], data[3]};
    for (size_t i = 4; i < data.size(); i += 4)
    {
        vector<uint32_t> next_prev = {data[i], data[i + 1], data[i + 2], data[i + 3]};
        uint32_t block[4] = {data[i], data[i + 1], data[i + 2], data[i + 3]};
        decrypt_block(block, S);
        for (int j = 0; j < 4; j++)
        {
            data[i + j] = block[j]^prev[j];
        }
        prev = next_prev;
    }
    data.erase(data.begin(), data.begin() + 4);
    return data;
}

void Party::createKeys()
{
    AutoSeededRandomPool rnd;
    private_key = Integer(rnd, 2, p - 2);
    public_key = calc(g, private_key, p);
}

void Party::getKeyFromSecret()
{
    size_t key_size = k / 8;
    vector<uint8_t> key(key_size);
    shared_secret.Encode(key.data(), key_size);
    S = keySchedule(key);
}

void print_data(vector<uint32_t> data)
{
    for (size_t i = 0; i < data.size(); i++)
        cout << hex << setw(8) << setfill('0') << data[i] << " ";
    cout << dec << endl;
}
int main()
{
    test_data_received();
}