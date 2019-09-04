#include <Mouse.h>

void setup()
{
    int i;
   
    Mouse.begin();
   
    delay(1000);
   
    for(i=0; i<30; i++)
    {
        Mouse.move(10, 0, 0);
        delay(10);
    }

    for(i=0; i<30; i++)
    {
        Mouse.move(0, 10, 0);
        delay(10);
    }

    for(i=0; i<30; i++)
    {
        Mouse.move(-10, 0, 0);
        delay(10);
    }
   
    for(i=0; i<30; i++)
    {
        Mouse.move(0, -10, 0);
        delay(10);
    }
   
   
   
}

void loop()
{
    Mouse.move(1, 0, 0);
    delay(1000);
    Mouse.move(-1, 0, 0);
    delay(1000);
}

