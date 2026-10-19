#define BASE_FORCE_THREADS_COUNT 1
#include "base/base.h"
#include "base/base.c"

#define Width 3
#define Height 5
BASE_ENTRY_POINT()
{
 int Matrix[3][5] = {0};

		for(int Y = 0; Y < Width; Y += 1)
			for(int X = 0; X < Height; X += 1)
				Matrix[Y][X] = X + 10*Y;
 
 return 0;
}
