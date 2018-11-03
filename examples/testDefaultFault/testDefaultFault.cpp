#include "GPIOlib.h"

using namespace GPIO;

int main()
{
	init();

	turnTo(3);
	delay(5000);

	turnTo(0);
	return 0;
}
