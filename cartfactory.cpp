#include <stdint.h>
#include "umdbase.h"
#include "genesis.h"
#include "sms.h"
#include "generic.h"
#include "noopcart.h"


#define CARTS_LEN 7

noopcart* noop = new noopcart();
generic* genericCart = new generic();

umdbase* carts[] = {
  new genesis(),
  new genesis(),
  new sms(),
  genericCart, // pce
  genericCart, // tg16
  genericCart, // snes
  genericCart, // sneslo
};

umdbase* getCart(uint8_t mode)
{
    if (mode < CARTS_LEN && mode >=0)
    {
        return carts[mode-1];
    }
    return noop;
}

