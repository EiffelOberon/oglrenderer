#ifndef COMPLEX_H
#define COMPLEX_H


struct complex
{
    vec2 mComponent;
};


complex mul(
    in complex c0,
    in complex c1)
{
    complex result;
    result.mComponent.x = c0.mComponent.x * c1.mComponent.x - c0.mComponent.y * c1.mComponent.y;
    result.mComponent.y = c0.mComponent.x * c1.mComponent.y + c0.mComponent.y * c1.mComponent.x;
    return result;
}


complex add(
    in complex c0,
    in complex c1)
{
    complex result;
    result.mComponent.x = c0.mComponent.x + c1.mComponent.x;
    result.mComponent.y = c0.mComponent.y + c1.mComponent.y;
    return result;
}


complex conj(
    in complex c)
{
    complex result;
    result.mComponent.x = c.mComponent.x;
    result.mComponent.y = -c.mComponent.y;
    return result;
}


#endif