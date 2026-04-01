#include <iostream>
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

public:
    Party(Integer P, Integer G)
    {
        p = P;
        g = G;
        AutoSeededRandomPool rnd;
        private_key = Integer(rnd,2,p-2);
        cout<< private_key << endl;
        public_key = calc(g, private_key, p);
    }

    Integer sendPublicKey()
    {
        return public_key;
    }

    void create_shared_secret(Integer key_recived)
    {
        shared_secret = calc(key_recived, private_key, p);
        cout << shared_secret << endl;
    }
};

Integer generate_prime(int bits=64)
{
    AutoSeededRandomPool rnd;
    Integer p;
    PrimeAndGenerator pg;
    pg.Generate(1,rnd,bits,bits-1);
    return pg.Prime();
}

int main()
{
    Integer p = generate_prime();
    Integer g = 5;
    cout<<p<<endl;
    Party a(p, g), b(p, g);
    b.create_shared_secret(a.sendPublicKey());
    a.create_shared_secret(b.sendPublicKey());
}