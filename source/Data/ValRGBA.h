#pragma once
#include <assert.h>

struct ValRGBA {
	int r,
		g,
		b,
		a;

	friend ValRGBA operator+ (ValRGBA _a, ValRGBA _b)
	{
		ValRGBA o;
		o.r = _a.r + _b.r;
		o.g = _a.g + _b.g;
		o.b = _a.b + _b.b;
		o.a = _a.a;

		return o;
	}

	friend ValRGBA operator-(ValRGBA _a, ValRGBA _b)
	{
		ValRGBA o;
		o.r = _a.r - _b.r;
		o.g = _a.g - _b.g;
		o.b = _a.b - _b.b;
		o.a = _a.a;

		return o;
	}

	friend ValRGBA operator*(ValRGBA _a, int _m)
	{
		ValRGBA o;
		o.r = _a.r * _m;
		o.g = _a.g * _m;
		o.b = _a.b * _m;
		o.a = _a.a;

		return o;
	}

	friend ValRGBA operator/(ValRGBA _a, int _d)
	{
		ValRGBA o;
		o.r = _a.r / _d;
		o.g = _a.g / _d;
		o.b = _a.b / _d;
		o.a = _a.a;

		return o;
	}
};


