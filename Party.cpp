#include "header.h"
#include <cryptopp/integer.h>
#include <cryptopp/osrng.h>
#include <cryptopp/nbtheory.h>

using namespace std;
using namespace CryptoPP;

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

class Party{
    Integer private_key;
    Integer public_key;
    Integer p, g;
    Integer shared_secret;
    int k = 128;
    vector<uint32_t> S;

public:
    Party()
    {
        AutoSeededRandomPool rnd;
        PrimeAndGenerator pg;
        pg.Generate(1, rnd, k, k - 1);
        p = pg.Prime();
        g = pg.Generator();
        cout<<"p: "<<p<<endl<<"g: "<<g<<endl;
        createKeys();
    }

    Party(Integer P, Integer G)
    {
        p = P;
        g = G;
        createKeys();
    }

    Integer getP()
    {
        return p;
    }
    Integer getG()
    {
        return g;
    }
    Integer sendPublicKey()
    {
        return public_key;
    }

    void create_shared_secret(Integer key_recived)
    {
        shared_secret = calc(key_recived, private_key, p);
        cout << "Shared secret: " << shared_secret << endl;
        getKeyFromSecret();
    }

    void encrypt(uint32_t Data[4])
    {
        encrypt_block(Data, S);
    }
    void decrypt(uint32_t Data[4])
    {
        decrypt_block(Data, S);
    }


private:
    void createKeys()
    {
        AutoSeededRandomPool rnd;
        private_key = Integer(rnd, 2, p - 2);
        public_key = calc(g, private_key, p);
    }

    void getKeyFromSecret()
    {
        size_t key_size = k/8;
        vector<uint8_t> key(key_size);
        shared_secret.Encode(key.data(), key_size);
        S = keySchedule(key);
    }

};

int main()
{
    Party a;
    Party b(a.getP(), a.getG());
    b.create_shared_secret(a.sendPublicKey());
    a.create_shared_secret(b.sendPublicKey());

    uint32_t myData[4] = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD};
    a.encrypt(myData);
    print_block(myData);
    b.decrypt(myData);
    print_block(myData);
}