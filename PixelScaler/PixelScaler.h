#pragma once

#include "FilterCommon.h"

#include "Brightness.h"
#include "Flip.h"
#include "hq.h"
#include "Interpolate.h"
#include "Interpolate4.h"
#include "Interpolate8.h"
#include "kreed.h"
#include "Kuwahara.h"
#include "lq.h"
#include "ReverseAA.h"
#include "Rotate.h"
#include "xbr.h"
#include "xbrz.h"

class PixelScaler
{

private:

	void Init(int srcx, int srcy, int scalex, int scaley)
	{
		_ScaleX = scalex;
		_ScaleY = scaley;

		SizeX = srcx * _ScaleX;
		SizeY = srcy * _ScaleY;

		free(ScaledImage);

		ScaledImage = New(SizeX, SizeY);
	}

	unsigned char* Buffer(int Length, unsigned char c)
	{
		auto Channels = 3;

		auto buffer = (unsigned char*)malloc(Length * Channels);

		for (auto i = 0; i < Length; i++)
		{
			auto index = i * Channels;

			buffer[index] = c;
			buffer[index + 1] = c;
			buffer[index + 2] = c;
		}

		return buffer;
	}

	unsigned char* New(int x, int y)
	{
		return Buffer(x * y, (unsigned char)0);
	}

	void Copy(unsigned char*& dst, unsigned char*& src, int Length)
	{
		memcpy(dst, src, Length * sizeof(unsigned char));
	}

public:

	unsigned char *ScaledImage = NULL;

	int SizeX = 0;
	int SizeY = 0;

	void Release()
	{
		free(ScaledImage);
	}

	void SetThreshold(bool threshold)
	{
		_Threshold = threshold;
	}

	void EPX(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				//   A      .---.
				//   |      |1|2|
				// C-S-B => |-+-|
				//   |      |3|4|
				//   D      .---.     

				auto S = CLR(Input, srcx, srcy, x, y);
				auto A = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto B = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto C = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto D = CLR(Input, srcx, srcy, x, y, 0, 1);

				int P[5];

				P[1] = IsLike(C, A) && IsNotLike(C, D) && IsNotLike(A, B) ? A : S;
				P[2] = IsLike(A, B) && IsNotLike(A, C) && IsNotLike(B, D) ? B : S;
				P[3] = IsLike(D, C) && IsNotLike(D, B) && IsNotLike(C, A) ? C : S;
				P[4] = IsLike(B, D) && IsNotLike(B, A) && IsNotLike(D, C) ? D : S;

				for (auto i = 1; i < 5; i++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, i, P[i]);
				}
			}
		}
	}

	void EPXB(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto c0 = CLR(Input, srcx, srcy, x, y, -1, -1);
				auto c1 = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto c2 = CLR(Input, srcx, srcy, x, y, 1, -1);
				auto c3 = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c4 = CLR(Input, srcx, srcy, x, y);
				auto c5 = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto c6 = CLR(Input, srcx, srcy, x, y, -1, 1);
				auto c7 = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto c8 = CLR(Input, srcx, srcy, x, y, 1, 1);

				int P[5];

				P[1] = P[2] = P[3] = P[4] = c4;

				if (
					IsNotLike(c3, c5) &&
					IsNotLike(c1, c7) && ( // diagonal
					(
						IsLike(c4, c3) ||
						IsLike(c4, c7) ||
						IsLike(c4, c5) ||
						IsLike(c4, c1) || ( // edge smoothing
						(
							IsNotLike(c0, c8) ||
							IsLike(c4, c6) ||
							IsLike(c4, c2)
							) && (
								IsNotLike(c6, c2) ||
								IsLike(c4, c0) ||
								IsLike(c4, c8)
								)
							)
						)
						)
					) {
					if (
						IsLike(c1, c3) && (
							IsNotLike(c4, c0) ||
							IsNotLike(c4, c8) ||
							IsNotLike(c1, c2) ||
							IsNotLike(c3, c6)
							)
						) {
						P[1] = Interpolate(c1, c3);
					}
					if (
						IsLike(c5, c1) && (
							IsNotLike(c4, c2) ||
							IsNotLike(c4, c6) ||
							IsNotLike(c5, c8) ||
							IsNotLike(c1, c0)
							)
						) {
						P[2] = Interpolate(c5, c1);
					}
					if (
						IsLike(c3, c7) && (
							IsNotLike(c4, c6) ||
							IsNotLike(c4, c2) ||
							IsNotLike(c3, c0) ||
							IsNotLike(c7, c8)
							)
						) {
						P[3] = Interpolate(c3, c7);
					}
					if (
						IsLike(c7, c5) && (
							IsNotLike(c4, c8) ||
							IsNotLike(c4, c0) ||
							IsNotLike(c7, c6) ||
							IsNotLike(c5, c2)
							)
						) {
						P[4] = Interpolate(c7, c5);
					}
				}

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void EPXC(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto c0 = CLR(Input, srcx, srcy, x, y, -1, -1);
				auto c1 = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto c2 = CLR(Input, srcx, srcy, x, y, 1, -1);
				auto c3 = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c4 = CLR(Input, srcx, srcy, x, y);
				auto c5 = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto c6 = CLR(Input, srcx, srcy, x, y, -1, 1);
				auto c7 = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto c8 = CLR(Input, srcx, srcy, x, y, 1, 1);

				int P[5];

				P[1] = P[2] = P[3] = P[4] = c4;

				if (IsNotLike(c3, c5) && IsNotLike(c7, c1)) {
					auto neq40 = IsNotLike(c4, c0);
					auto neq41 = IsNotLike(c4, c1);
					auto neq42 = IsNotLike(c4, c2);
					auto neq43 = IsNotLike(c4, c3);
					auto neq45 = IsNotLike(c4, c5);
					auto neq46 = IsNotLike(c4, c6);
					auto neq47 = IsNotLike(c4, c7);
					auto neq48 = IsNotLike(c4, c8);

					auto eq13 = IsLike(c1, c3) && (neq40 || neq48 || IsNotLike(c1, c2) || IsNotLike(c3, c6));
					auto eq37 = IsLike(c3, c7) && (neq46 || neq42 || IsNotLike(c3, c0) || IsNotLike(c7, c8));
					auto eq75 = IsLike(c7, c5) && (neq48 || neq40 || IsNotLike(c7, c6) || IsNotLike(c5, c2));
					auto eq51 = IsLike(c5, c1) && (neq42 || neq46 || IsNotLike(c5, c8) || IsNotLike(c1, c0));
					if (
						(!neq40) ||
						(!neq41) ||
						(!neq42) ||
						(!neq43) ||
						(!neq45) ||
						(!neq46) ||
						(!neq47) ||
						(!neq48)
						) {

						int c3A;

						if ((eq13 && neq46) && (eq37 && neq40))
							c3A = Interpolate(c3, c1, c7);
						else if (eq13 && neq46)
							c3A = Interpolate(c3, c1);
						else if (eq37 && neq40)
							c3A = Interpolate(c3, c7);
						else
							c3A = c4;

						int c7B;
						if ((eq37 && neq48) && (eq75 && neq46))
							c7B = Interpolate(c7, c3, c5);
						else if (eq37 && neq48)
							c7B = Interpolate(c7, c3);
						else if (eq75 && neq46)
							c7B = Interpolate(c7, c5);
						else
							c7B = c4;

						int c5C;
						if ((eq75 && neq42) && (eq51 && neq48))
							c5C = Interpolate(c5, c1, c7);
						else if (eq75 && neq42)
							c5C = Interpolate(c5, c7);
						else if (eq51 && neq48)
							c5C = Interpolate(c5, c1);
						else
							c5C = c4;

						int c1D;

						if ((eq51 && neq40) && (eq13 && neq42))
							c1D = Interpolate(c1, c3, c5);
						else if (eq51 && neq40)
							c1D = Interpolate(c1, c5);
						else if (eq13 && neq42)
							c1D = Interpolate(c1, c3);
						else
							c1D = c4;

						if (eq13)
							P[1] = Interpolate(c1, c3);
						if (eq51)
							P[2] = Interpolate(c5, c1);
						if (eq37)
							P[3] = Interpolate(c3, c7);
						if (eq75)
							P[4] = Interpolate(c7, c5);

						P[1] = Interpolate(P[1], c1D, c3A, c4, 5, 1, 1, 1);
						P[2] = Interpolate(P[2], c7B, c5C, c4, 5, 1, 1, 1);
						P[3] = Interpolate(P[3], c3A, c7B, c4, 5, 1, 1, 1);
						P[4] = Interpolate(P[4], c5C, c1D, c4, 5, 1, 1, 1);

					}
					else {

						if (eq13)
							P[1] = Interpolate(c1, c3);
						if (eq51)
							P[2] = Interpolate(c5, c1);
						if (eq37)
							P[3] = Interpolate(c3, c7);
						if (eq75)
							P[4] = Interpolate(c7, c5);

						P[1] = Interpolate(c4, P[1], 3, 1);
						P[2] = Interpolate(c4, P[2], 3, 1);
						P[3] = Interpolate(c4, P[3], 3, 1);
						P[4] = Interpolate(c4, P[4], 3, 1);
					}
				}

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void EPX3(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 3, 3);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto c0 = CLR(Input, srcx, srcy, x, y, -1, -1);
				auto c1 = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto c2 = CLR(Input, srcx, srcy, x, y, 1, -1);
				auto c3 = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c4 = CLR(Input, srcx, srcy, x, y);
				auto c5 = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto c6 = CLR(Input, srcx, srcy, x, y, -1, 1);
				auto c7 = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto c8 = CLR(Input, srcx, srcy, x, y, 1, 1);

				int P[10];

				P[1] = P[2] = P[3] = P[4] = P[5] = P[6] = P[7] = P[8] = P[9] = c4;

				if (IsNotLike(c3, c5) && IsNotLike(c7, c1)) {
					auto neq40 = IsNotLike(c4, c0);
					auto neq41 = IsNotLike(c4, c1);
					auto neq42 = IsNotLike(c4, c2);
					auto neq43 = IsNotLike(c4, c3);
					auto neq45 = IsNotLike(c4, c5);
					auto neq46 = IsNotLike(c4, c6);
					auto neq47 = IsNotLike(c4, c7);
					auto neq48 = IsNotLike(c4, c8);

					auto eq13 = IsLike(c1, c3) && (neq40 || neq48 || IsNotLike(c1, c2) || IsNotLike(c3, c6));
					auto eq37 = IsLike(c3, c7) && (neq46 || neq42 || IsNotLike(c3, c0) || IsNotLike(c7, c8));
					auto eq75 = IsLike(c7, c5) && (neq48 || neq40 || IsNotLike(c7, c6) || IsNotLike(c5, c2));
					auto eq51 = IsLike(c5, c1) && (neq42 || neq46 || IsNotLike(c5, c8) || IsNotLike(c1, c0));
					if (
						(!neq40) ||
						(!neq41) ||
						(!neq42) ||
						(!neq43) ||
						(!neq45) ||
						(!neq46) ||
						(!neq47) ||
						(!neq48)
						) {
						if (eq13)
							P[1] = Interpolate(c1, c3);
						if (eq51)
							P[3] = Interpolate(c5, c1);
						if (eq37)
							P[7] = Interpolate(c3, c7);
						if (eq75)
							P[9] = Interpolate(c7, c5);

						if ((eq51 && neq40) && (eq13 && neq42))
							P[2] = Interpolate(c1, c3, c5);
						else if (eq51 && neq40)
							P[2] = Interpolate(c1, c5);
						else if (eq13 && neq42)
							P[2] = Interpolate(c1, c3);

						if ((eq13 && neq46) && (eq37 && neq40))
							P[4] = Interpolate(c3, c1, c7);
						else if (eq13 && neq46)
							P[4] = Interpolate(c3, c1);
						else if (eq37 && neq40)
							P[4] = Interpolate(c3, c7);

						if ((eq75 && neq42) && (eq51 && neq48))
							P[6] = Interpolate(c5, c1, c7);
						else if (eq75 && neq42)
							P[6] = Interpolate(c5, c7);
						else if (eq51 && neq48)
							P[6] = Interpolate(c5, c1);

						if ((eq37 && neq48) && (eq75 && neq46))
							P[8] = Interpolate(c7, c3, c5);
						else if (eq75 && neq46)
							P[8] = Interpolate(c7, c5);
						else if (eq37 && neq48)
							P[8] = Interpolate(c7, c3);

					}
					else {
						if (eq13)
							P[1] = Interpolate(c1, c3);
						if (eq51)
							P[3] = Interpolate(c5, c1);
						if (eq37)
							P[7] = Interpolate(c3, c7);
						if (eq75)
							P[9] = Interpolate(c7, c5);
					}
				}

				for (auto Pixel = 1; Pixel < 10; Pixel++)
				{
					Write9RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void Scale4X(unsigned char*& Input, int srcx, int srcy)
	{
		auto Channels = 3;

		EPX(Input, srcx, srcy);

		auto temp = New(SizeX, SizeY);

		Copy(temp, ScaledImage, SizeX * SizeY * Channels);

		EPX(temp, SizeX, SizeY);

		_ScaleX = 4;
		_ScaleY = 4;

		free(temp);
	}

	void Scale3X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 3, 3);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				//          .-----.
				// A B C    |1|2|3|
				//  \|/     |-+-+-|
				// D-E-F => |4|5|6|
				//  /|\     |-+-+-|
				// G H I    |7|8|9|
				//          .-----.

				auto E = CLR(Input, srcx, srcy, x, y);
				auto A = CLR(Input, srcx, srcy, x, y, -1, -1);
				auto B = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto C = CLR(Input, srcx, srcy, x, y, 1, -1);
				auto D = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto F = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto G = CLR(Input, srcx, srcy, x, y, -1, 1);
				auto H = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto I = CLR(Input, srcx, srcy, x, y, 1, 1);

				int P[10];

				P[1] = IsLike(D, B) && IsNotLike(D, H) && IsNotLike(B, F) ? D : E;
				P[2] = (IsLike(D, B) && IsNotLike(D, H) && IsNotLike(B, F) && IsNotLike(E, C)) || (IsLike(B, F) && IsNotLike(B, D) && IsNotLike(F, H) && IsNotLike(E, A)) ? B : E;
				P[3] = IsLike(B, F) && IsNotLike(B, D) && IsNotLike(F, H) ? F : E;
				P[4] = (IsLike(H, D) && IsNotLike(H, F) && IsNotLike(D, B) && IsNotLike(E, A)) || (IsLike(D, B) && IsNotLike(D, H) && IsNotLike(B, F) && IsNotLike(E, G)) ? D : E;
				P[5] = E;
				P[6] = (IsLike(B, F) && IsNotLike(B, D) && IsNotLike(F, H) && IsNotLike(E, I)) || (IsLike(F, H) && IsNotLike(F, B) && IsNotLike(H, D) && IsNotLike(E, C)) ? F : E;
				P[7] = IsLike(H, D) && IsNotLike(H, F) && IsNotLike(D, B) ? D : E;
				P[8] = (IsLike(F, H) && IsNotLike(F, B) && IsNotLike(H, D) && IsNotLike(E, G)) || (IsLike(H, D) && IsNotLike(H, F) && IsNotLike(D, B) && IsNotLike(E, I)) ? H : E;
				P[9] = IsLike(F, H) && IsNotLike(F, B) && IsNotLike(H, D) ? F : E;

				for (auto i = 1; i < 10; i++)
				{
					Write9RGB(ScaledImage, srcx, srcy, x, y, i, P[i]);
				}
			}
		}
	}

	void Magnify(unsigned char*& Input, int srcx, int srcy, int Magnification)
	{
		Init(srcx, srcy, Magnification, Magnification);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				WriteMagnify(Input, ScaledImage, srcx, srcy, x, y);
			}
		}
	}

	void Mag2EPX(unsigned char*& Input, int srcx, int srcy)
	{
		auto Channels = 3;

		Magnify(Input, srcx, srcy, 2);

		auto temp = New(SizeX, SizeY);

		Copy(temp, ScaledImage, SizeX * SizeY * Channels);

		EPX(temp, SizeX, SizeY);

		_ScaleX = 4;
		_ScaleY = 4;

		free(temp);
	}

	void EPXMag2(unsigned char*& Input, int srcx, int srcy)
	{
		auto Channels = 3;

		EPX(Input, srcx, srcy);

		auto temp = New(SizeX, SizeY);

		Copy(temp, ScaledImage, SizeX * SizeY * Channels);

		Magnify(temp, SizeX, SizeY, 2);

		_ScaleX = 4;
		_ScaleY = 4;

		free(temp);
	}

	void Nearest(unsigned char*& Input, int srcx, int srcy)
	{
		auto Channels = 3;

		Init(srcx, srcy, 2, 2);

		int x_ratio = (int)((srcx << 16) / SizeX) + 1;
		int y_ratio = (int)((srcy << 16) / SizeY) + 1;

		for (int i = 0; i < SizeY; i++)
		{
			for (int j = 0; j < SizeX; j++)
			{
				auto x2 = ((j * x_ratio) >> 16);
				auto y2 = ((i * y_ratio) >> 16);

				auto dst = (i * SizeX + j) * Channels;
				auto src = (y2 * srcx + x2) * Channels;

				for (auto Channel = 0; Channel < Channels; Channel++)
				{
					ScaledImage[dst + Channel] = Input[src + Channel];
				}
			}
		}
	}

	void Bilinear(unsigned char*& Input, int srcx, int srcy)
	{
		auto Channels = 3;

		Init(srcx, srcy, 2, 2);

		int x_ratio = (int)((srcx << 16) / SizeX) + 1;
		int y_ratio = (int)((srcy << 16) / SizeY) + 1;

		int offset = 0;

		for (int i = 0; i < SizeY; i++) {

			for (int j = 0; j < SizeX; j++) {

				auto x = (x_ratio * j) >> 16;
				auto y = (y_ratio * i) >> 16;

				auto x_diff = (double)(((x_ratio * j) >> 16) - x);
				auto y_diff = (double)(((y_ratio * i) >> 16) - y);

				for (auto Channel = 0; Channel < Channels; Channel++)
				{
					auto index = y * srcx + x;

					auto A = Input[index * Channels + Channel];
					auto B = Input[(index + 1) * Channels + Channel];
					auto C = Input[(index + srcx) * Channels + Channel];
					auto D = Input[(index + srcx + 1) * Channels + Channel];

					// Y = A(1-w)(1-h) + B(w)(1-h) + C(h)(1-w) + Dwh
					auto color = (unsigned char)(
						A*(1 - x_diff)*(1 - y_diff) + B*(x_diff)*(1 - y_diff) +
						C*(y_diff)*(1 - x_diff) + D*(x_diff*y_diff)
						);

					ScaledImage[offset * Channels + Channel] = color;
				}

				offset++;
			}
		}
	}

	void LQ2X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto c0 = CLR(Input, srcx, srcy, x, y, -1, -1);
				auto c1 = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto c2 = CLR(Input, srcx, srcy, x, y, 1, -1);
				auto c3 = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c4 = CLR(Input, srcx, srcy, x, y);
				auto c5 = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto c6 = CLR(Input, srcx, srcy, x, y, -1, 1);
				auto c7 = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto c8 = CLR(Input, srcx, srcy, x, y, 1, 1);

				unsigned char pattern = 0;
				if ((IsNotLike(c4, c0)))
					pattern |= 1;
				if ((IsNotLike(c4, c1)))
					pattern |= 2;
				if ((IsNotLike(c4, c2)))
					pattern |= 4;
				if ((IsNotLike(c4, c3)))
					pattern |= 8;
				if ((IsNotLike(c4, c5)))
					pattern |= 16;
				if ((IsNotLike(c4, c6)))
					pattern |= 32;
				if ((IsNotLike(c4, c7)))
					pattern |= 64;
				if ((IsNotLike(c4, c8)))
					pattern |= 128;

				int P[5];

				Lq2xKernel(pattern, c0, c1, c2, c3, c4, c5, c6, c7, c8, P);

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void LQ3X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 3, 3);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto c0 = CLR(Input, srcx, srcy, x, y, -1, -1);
				auto c1 = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto c2 = CLR(Input, srcx, srcy, x, y, 1, -1);
				auto c3 = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c4 = CLR(Input, srcx, srcy, x, y);
				auto c5 = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto c6 = CLR(Input, srcx, srcy, x, y, -1, 1);
				auto c7 = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto c8 = CLR(Input, srcx, srcy, x, y, 1, 1);

				unsigned char pattern = 0;
				if ((IsNotLike(c4, c0)))
					pattern |= 1;
				if ((IsNotLike(c4, c1)))
					pattern |= 2;
				if ((IsNotLike(c4, c2)))
					pattern |= 4;
				if ((IsNotLike(c4, c3)))
					pattern |= 8;
				if ((IsNotLike(c4, c5)))
					pattern |= 16;
				if ((IsNotLike(c4, c6)))
					pattern |= 32;
				if ((IsNotLike(c4, c7)))
					pattern |= 64;
				if ((IsNotLike(c4, c8)))
					pattern |= 128;

				int P[10];

				Lq3xKernel(pattern, c0, c1, c2, c3, c4, c5, c6, c7, c8, P);

				for (auto Pixel = 1; Pixel < 10; Pixel++)
				{
					Write9RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void LQ4X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 4, 4);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto c0 = CLR(Input, srcx, srcy, x, y, -1, -1);
				auto c1 = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto c2 = CLR(Input, srcx, srcy, x, y, 1, -1);
				auto c3 = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c4 = CLR(Input, srcx, srcy, x, y);
				auto c5 = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto c6 = CLR(Input, srcx, srcy, x, y, -1, 1);
				auto c7 = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto c8 = CLR(Input, srcx, srcy, x, y, 1, 1);

				unsigned char pattern = 0;
				if ((IsNotLike(c4, c0)))
					pattern |= 1;
				if ((IsNotLike(c4, c1)))
					pattern |= 2;
				if ((IsNotLike(c4, c2)))
					pattern |= 4;
				if ((IsNotLike(c4, c3)))
					pattern |= 8;
				if ((IsNotLike(c4, c5)))
					pattern |= 16;
				if ((IsNotLike(c4, c6)))
					pattern |= 32;
				if ((IsNotLike(c4, c7)))
					pattern |= 64;
				if ((IsNotLike(c4, c8)))
					pattern |= 128;

				int P[17];

				Lq4xKernel(pattern, c0, c1, c2, c3, c4, c5, c6, c7, c8, P);

				for (auto Pixel = 1; Pixel < 17; Pixel++)
				{
					Write16RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void Eagle2X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				int C[9];

				C[0] = CLR(Input, srcx, srcy, x, y, -1, -1);
				C[1] = CLR(Input, srcx, srcy, x, y, 0, -1);
				C[2] = CLR(Input, srcx, srcy, x, y, 1, -1);
				C[3] = CLR(Input, srcx, srcy, x, y, -1, 0);
				C[4] = CLR(Input, srcx, srcy, x, y, 0, 0);
				C[5] = CLR(Input, srcx, srcy, x, y, 1, 0);
				C[6] = CLR(Input, srcx, srcy, x, y, -1, 1);
				C[7] = CLR(Input, srcx, srcy, x, y, 0, 1);
				C[8] = CLR(Input, srcx, srcy, x, y, 1, 1);

				int P[5];

				P[1] = (IsLike(C[1], C[0]) && IsLike(C[1], C[3])) ? Interpolate(C[1], C[0], C[3]) : C[4];
				P[2] = (IsLike(C[2], C[1]) && IsLike(C[2], C[5])) ? Interpolate(C[2], C[1], C[5]) : C[4];
				P[3] = (IsLike(C[6], C[3]) && IsLike(C[6], C[7])) ? Interpolate(C[6], C[3], C[7]) : C[4];
				P[4] = (IsLike(C[7], C[5]) && IsLike(C[7], C[8])) ? Interpolate(C[7], C[5], C[8]) : C[4];

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void Eagle3X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 3, 3);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				//   C         P
				//
				//          +-----+
				// 0 1 2    |1|2|3|
				//  \|/     |-+-+-|
				// 3-4-5 => |4|5|6|
				//  /|\     |-+-+-|
				// 6 7 8    |7|8|9|
				//          +-----+

				int C[9];

				C[0] = CLR(Input, srcx, srcy, x, y, -1, -1);
				C[1] = CLR(Input, srcx, srcy, x, y, 0, -1);
				C[2] = CLR(Input, srcx, srcy, x, y, 1, -1);
				C[3] = CLR(Input, srcx, srcy, x, y, -1, 0);
				C[4] = CLR(Input, srcx, srcy, x, y, 0, 0);
				C[5] = CLR(Input, srcx, srcy, x, y, 1, 0);
				C[6] = CLR(Input, srcx, srcy, x, y, -1, 1);
				C[7] = CLR(Input, srcx, srcy, x, y, 0, 1);
				C[8] = CLR(Input, srcx, srcy, x, y, 1, 1);

				int P[10];

				P[1] = (IsLike(C[0], C[1]) && IsLike(C[0], C[3])) ? Interpolate(C[0], C[1], C[3]) : C[4];
				P[2] = (IsLike(C[0], C[1]) && IsLike(C[0], C[3]) && IsLike(C[2], C[1]) && IsLike(C[2], C[5])) ? Interpolate(Interpolate(C[0], C[1], C[3]), Interpolate(C[2], C[1], C[5])) : C[4];
				P[3] = (IsLike(C[2], C[1]) && IsLike(C[2], C[5])) ? Interpolate(C[2], C[1], C[5]) : C[4];
				P[4] = (IsLike(C[0], C[1]) && IsLike(C[0], C[3]) && IsLike(C[6], C[7]) && IsLike(C[6], C[3])) ? Interpolate(Interpolate(C[0], C[1], C[3]), Interpolate(C[6], C[3], C[7])) : C[4];
				P[5] = C[4];
				P[6] = (IsLike(C[2], C[1]) && IsLike(C[2], C[5]) && IsLike(C[8], C[5]) && IsLike(C[8], C[7])) ? Interpolate(Interpolate(C[2], C[1], C[5]), Interpolate(C[8], C[5], C[7])) : C[4];
				P[7] = (IsLike(C[6], C[3]) && IsLike(C[6], C[7])) ? Interpolate(C[6], C[3], C[7]) : C[4];
				P[8] = (IsLike(C[6], C[7]) && IsLike(C[6], C[3]) && IsLike(C[8], C[5]) && IsLike(C[8], C[7])) ? Interpolate(Interpolate(C[6], C[7], C[3]), Interpolate(C[8], C[5], C[7])) : C[4];
				P[9] = (IsLike(C[8], C[5]) && IsLike(C[8], C[7])) ? Interpolate(C[8], C[5], C[7]) : C[4];

				for (auto Pixel = 1; Pixel < 10; Pixel++)
				{
					Write9RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void Eagle3XB(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 3, 3);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				//   C         P
				//
				//          +-----+
				// 0 1 2    |1|2|3|
				//  \|/     |-+-+-|
				// 3-4-5 => |4|5|6|
				//  /|\     |-+-+-|
				// 6 7 8    |7|8|9|
				//          +-----+

				int C[9];

				C[0] = CLR(Input, srcx, srcy, x, y, -1, -1);
				C[1] = CLR(Input, srcx, srcy, x, y, 0, -1);
				C[2] = CLR(Input, srcx, srcy, x, y, 1, -1);
				C[3] = CLR(Input, srcx, srcy, x, y, -1, 0);
				C[4] = CLR(Input, srcx, srcy, x, y, 0, 0);
				C[5] = CLR(Input, srcx, srcy, x, y, 1, 0);
				C[6] = CLR(Input, srcx, srcy, x, y, -1, 1);
				C[7] = CLR(Input, srcx, srcy, x, y, 0, 1);
				C[8] = CLR(Input, srcx, srcy, x, y, 1, 1);

				int P[10];

				P[1] = (IsLike(C[0], C[1]) && IsLike(C[0], C[3])) ? Interpolate(C[0], C[1], C[3]) : C[4];
				P[2] = C[4];
				P[3] = (IsLike(C[2], C[1]) && IsLike(C[2], C[5])) ? Interpolate(C[2], C[1], C[5]) : C[4];
				P[4] = C[4];
				P[5] = C[4];
				P[6] = C[4];
				P[7] = (IsLike(C[6], C[3]) && IsLike(C[6], C[7])) ? Interpolate(C[6], C[3], C[7]) : C[4];
				P[8] = C[4];
				P[9] = (IsLike(C[8], C[5]) && IsLike(C[8], C[7])) ? Interpolate(C[8], C[5], C[7]) : C[4];

				for (auto Pixel = 1; Pixel < 10; Pixel++)
				{
					Write9RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void Super2X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto n = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto w = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c = CLR(Input, srcx, srcy, x, y, 0, 0);
				auto e = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto s = CLR(Input, srcx, srcy, x, y, 0, 1);

				auto cx = c;
				auto wx = w;
				auto ex = e;

				auto cw = _Mixpal(c, w);
				auto ce = _Mixpal(c, e);

				int P[5];

				P[1] = (((IsLike(w, n)) && (IsNotLike(n, e)) && (IsNotLike(w, s))) ? wx : cw);
				P[2] = (((IsLike(n, e)) && (IsNotLike(n, w)) && (IsNotLike(e, s))) ? ex : ce);
				P[3] = (((IsLike(w, s)) && (IsNotLike(w, n)) && (IsNotLike(s, e))) ? wx : cw);
				P[4] = (((IsLike(s, e)) && (IsNotLike(w, s)) && (IsNotLike(n, e))) ? ex : ce);

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void Ultra2X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto n = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto w = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c = CLR(Input, srcx, srcy, x, y, 0, 0);
				auto e = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto s = CLR(Input, srcx, srcy, x, y, 0, 1);

				auto cx = c;
				auto wx = w;
				auto ex = e;

				auto cw = _Mixpal(c, w);
				auto ce = _Mixpal(c, e);

				int P[5];

				P[1] = (((IsLike(w, n)) && (IsNotLike(n, e)) && (IsNotLike(w, s))) ? wx : cw);
				P[2] = (((IsLike(n, e)) && (IsNotLike(n, w)) && (IsNotLike(e, s))) ? ex : ce);
				P[3] = (((IsLike(w, s)) && (IsNotLike(w, n)) && (IsNotLike(s, e))) ? wx : cw);
				P[4] = (((IsLike(s, e)) && (IsNotLike(w, s)) && (IsNotLike(n, e))) ? ex : ce);

				P[1] = _Unmix(P[1], cx);
				P[2] = _Unmix(P[2], cx);
				P[3] = _Unmix(P[3], cx);
				P[4] = _Unmix(P[4], cx);

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void DES2X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto n = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto w = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c = CLR(Input, srcx, srcy, x, y, 0, 0);
				auto e = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto s = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto se = CLR(Input, srcx, srcy, x, y, 1, 1);

				int P[5];

				auto d1 = (((IsLike(w, n)) && (IsNotLike(n, e)) && (IsNotLike(w, s))) ? w : c);
				auto d2 = (((IsLike(n, e)) && (IsNotLike(n, w)) && (IsNotLike(e, s))) ? e : c);
				auto d3 = (((IsLike(w, s)) && (IsNotLike(w, n)) && (IsNotLike(s, e))) ? w : c);
				auto d4 = (((IsLike(s, e)) && (IsNotLike(w, s)) && (IsNotLike(n, e))) ? e : c);

				auto cx = c;
				auto ce = Interpolate(c, e, 3, 1);
				auto cs = Interpolate(c, s, 3, 1);
				auto cse = Interpolate(c, se, 3, 1);

				P[1] = Interpolate(d1, cx, 3, 1);
				P[2] = Interpolate(d2, ce, 3, 1);
				P[3] = Interpolate(d3, cs, 3, 1);
				P[4] = Interpolate(d4, cse, 3, 1);

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void TV2X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				int P[5];

				auto pixel = CLR(Input, srcx, srcy, x, y);

				auto subPixel = RGBINT((Red(pixel) * 5) >> 3, (Green(pixel) * 5) >> 3, (Blue(pixel) * 5) >> 3);

				P[1] = pixel;
				P[2] = pixel;
				P[3] = subPixel;
				P[4] = subPixel;

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void TV3X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 3, 3);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				//   C         P
				//
				//          +-----+
				// 0 1 2    |1|2|3|
				//  \|/     |-+-+-|
				// 3-4-5 => |4|5|6|
				//  /|\     |-+-+-|
				// 6 7 8    |7|8|9|
				//          +-----+

				int P[10];

				auto pixel = CLR(Input, srcx, srcy, x, y);

				auto R = Red(pixel) * 5;
				auto G = Green(pixel) * 5;
				auto B = Blue(pixel) * 5;

				auto subPixel = RGBINT(R >> 3, G >> 3, B >> 3);
				auto subPixel2 = RGBINT(R >> 4, G >> 4, B >> 4);

				P[1] = pixel;
				P[2] = pixel;
				P[3] = pixel;
				P[4] = subPixel;
				P[5] = subPixel;
				P[6] = subPixel;
				P[7] = subPixel2;
				P[8] = subPixel2;
				P[9] = subPixel2;

				for (auto Pixel = 1; Pixel < 10; Pixel++)
				{
					Write9RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void RGB2X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				int P[5];

				auto pixel = CLR(Input, srcx, srcy, x, y);
				auto subPixel = (int)((pixel * 5) >> 3);

				P[1] = RGBINT(Red(pixel), 0, 0);
				P[2] = RGBINT(0, Green(pixel), 0);
				P[3] = RGBINT(0, 0, Blue(pixel));
				P[4] = pixel;

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void RGB3X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 3, 3);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				//   C         P
				//
				//          +-----+
				// 0 1 2    |1|2|3|
				//  \|/     |-+-+-|
				// 3-4-5 => |4|5|6|
				//  /|\     |-+-+-|
				// 6 7 8    |7|8|9|
				//          +-----+

				int P[10];

				auto pixel = CLR(Input, srcx, srcy, x, y);

				P[1] = pixel;
				P[2] = RGBINT(0, Green(pixel), 0);
				P[3] = RGBINT(0, 0, Blue(pixel));
				P[4] = RGBINT(0, 0, Blue(pixel));
				P[5] = pixel;
				P[6] = RGBINT(Red(pixel), 0, 0);
				P[7] = RGBINT(Red(pixel), 0, 0);
				P[8] = RGBINT(0, Green(pixel), 0);
				P[9] = pixel;

				for (auto Pixel = 1; Pixel < 10; Pixel++)
				{
					Write9RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void Horiz2X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				int P[5];

				auto pixel = CLR(Input, srcx, srcy, x, y);

				auto R = Red(pixel) >> 1;
				auto G = Green(pixel) >> 1;
				auto B = Blue(pixel) >> 1;

				auto subPixel = RGBINT(R, G, B);

				P[1] = pixel;
				P[2] = pixel;
				P[3] = subPixel;
				P[4] = subPixel;

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void Horiz3X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 3, 3);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				int P[10];

				auto pixel = CLR(Input, srcx, srcy, x, y);

				auto R = Red(pixel) >> 1;
				auto G = Green(pixel) >> 1;
				auto B = Blue(pixel) >> 1;

				auto subPixel = RGBINT(R, G, B);
				auto subPixel2 = RGBINT(R >> 1, G >> 1, B >> 1);

				P[1] = P[2] = P[3] = pixel;
				P[4] = P[5] = P[6] = subPixel;
				P[7] = P[8] = P[9] = subPixel;

				for (auto Pixel = 1; Pixel < 10; Pixel++)
				{
					Write9RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void VertScan2X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				int P[5];

				auto pixel = CLR(Input, srcx, srcy, x, y);

				auto R = Red(pixel) >> 1;
				auto G = Green(pixel) >> 1;
				auto B = Blue(pixel) >> 1;

				auto subPixel = RGBINT(R, G, B);

				P[1] = pixel;
				P[2] = subPixel;
				P[3] = pixel;
				P[4] = subPixel;

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void AdvInterp2X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto c1 = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto c3 = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c4 = CLR(Input, srcx, srcy, x, y, 0, 0);
				auto c5 = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto c7 = CLR(Input, srcx, srcy, x, y, 0, 1);

				int P[5];

				P[1] = P[2] = P[3] = P[4] = c4;

				if (IsNotLike(c1, c7) && IsNotLike(c3, c5))
				{
					P[1] = (IsLike(c3, c1)) ? Interpolate(Interpolate(c1, c3), c4, 5, 3) : c4;
					P[2] = (IsLike(c5, c1)) ? Interpolate(Interpolate(c1, c5), c4, 5, 3) : c4;
					P[3] = (IsLike(c3, c7)) ? Interpolate(Interpolate(c7, c3), c4, 5, 3) : c4;
					P[3] = (IsLike(c5, c7)) ? Interpolate(Interpolate(c7, c5), c4, 5, 3) : c4;
				}

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void AdvInterp3X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 3, 3);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto c0 = CLR(Input, srcx, srcy, x, y, -1, -1);
				auto c1 = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto c2 = CLR(Input, srcx, srcy, x, y, 1, -1);
				auto c3 = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c4 = CLR(Input, srcx, srcy, x, y);
				auto c5 = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto c6 = CLR(Input, srcx, srcy, x, y, -1, 1);
				auto c7 = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto c8 = CLR(Input, srcx, srcy, x, y, 1, 1);

				int P[10];

				P[1] = P[2] = P[3] = P[4] = P[5] = P[6] = P[7] = P[8] = P[9] = c4;

				if (IsNotLike(c1, c7) && IsNotLike(c3, c5)) {
					if (IsLike(c3, c1)) {
						P[1] = Interpolate(Interpolate(c3, c1), c4, 5, 3);
					}
					if (IsLike(c1, c5)) {
						P[3] = Interpolate(Interpolate(c5, c1), c4, 5, 3);
					}
					if (IsLike(c3, c7)) {
						P[7] = Interpolate(Interpolate(c3, c7), c4, 5, 3);
					}
					if (IsLike(c7, c5)) {
						P[9] = Interpolate(Interpolate(c7, c5), c4, 5, 3);
					}

					if (
						(IsLike(c3, c1) && IsNotLike(c4, c2)) &&
						(IsLike(c5, c1) && IsNotLike(c4, c0))
						)
						P[2] = Interpolate(c1, c3, c5);
					else if (IsLike(c3, c1) && IsNotLike(c4, c2))
						P[2] = Interpolate(c3, c1);
					else if (IsLike(c5, c1) && IsNotLike(c4, c0))
						P[2] = Interpolate(c5, c1);

					if (
						(IsLike(c3, c1) && IsNotLike(c4, c6)) &&
						(IsLike(c3, c7) && IsNotLike(c4, c0))
						)
						P[4] = Interpolate(c3, c1, c7);
					else if (IsLike(c3, c1) && IsNotLike(c4, c6))
						P[4] = Interpolate(c3, c1);
					else if (IsLike(c3, c7) && IsNotLike(c4, c0))
						P[4] = Interpolate(c3, c7);

					if (
						(IsLike(c5, c1) && IsNotLike(c4, c8)) &&
						(IsLike(c5, c7) && IsNotLike(c4, c2))
						)
						P[6] = Interpolate(c5, c1, c7);
					else if (IsLike(c5, c1) && IsNotLike(c4, c8))
						P[6] = Interpolate(c5, c1);
					else if (IsLike(c5, c7) && IsNotLike(c4, c2))
						P[6] = Interpolate(c5, c7);

					if (
						(IsLike(c3, c7) && IsNotLike(c4, c8)) &&
						(IsLike(c5, c7) && IsNotLike(c4, c6))
						)
						P[8] = Interpolate(c7, c3, c5);
					else if (IsLike(c3, c7) && IsNotLike(c4, c8))
						P[8] = Interpolate(c3, c7);
					else if (IsLike(c5, c7) && IsNotLike(c4, c6))
						P[8] = Interpolate(c5, c7);
				}

				for (auto Pixel = 1; Pixel < 10; Pixel++)
				{
					Write9RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void SCL2X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto n = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto w = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c = CLR(Input, srcx, srcy, x, y, 0, 0);
				auto e = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto s = CLR(Input, srcx, srcy, x, y, 0, 1);

				int P[5];

				P[1] = (((IsLike(w, n)) && (IsNotLike(n, e)) && (IsNotLike(w, s))) ? w : c);
				P[2] = (((IsLike(n, e)) && (IsNotLike(n, w)) && (IsNotLike(e, s))) ? e : c);
				P[3] = (((IsLike(w, s)) && (IsNotLike(w, n)) && (IsNotLike(s, e))) ? w : c);
				P[4] = (((IsLike(s, e)) && (IsNotLike(w, s)) && (IsNotLike(n, e))) ? e : c);

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void XBR2X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				/*
				A1 B1 C1
				A0 PA PB PC C4    P1 P2
				D0 PD PE PF F4 => --+--
				G0 PG PH PI I4    P3 P4
				G5 H5 I5
				*/

				auto a1 = CLR(Input, srcx, srcy, x, y, -1, -2);
				auto b1 = CLR(Input, srcx, srcy, x, y, 0, -2);
				auto c1 = CLR(Input, srcx, srcy, x, y, 1, -2);

				auto a0 = CLR(Input, srcx, srcy, x, y, -2, -1);
				auto pa = CLR(Input, srcx, srcy, x, y, -1, -1);
				auto pb = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto pc = CLR(Input, srcx, srcy, x, y, 1, -1);
				auto c4 = CLR(Input, srcx, srcy, x, y, 2, -1);

				auto d0 = CLR(Input, srcx, srcy, x, y, -2, 0);
				auto pd = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto pe = CLR(Input, srcx, srcy, x, y, 0, 0);
				auto pf = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto f4 = CLR(Input, srcx, srcy, x, y, 2, 0);

				auto g0 = CLR(Input, srcx, srcy, x, y, -2, 1);
				auto pg = CLR(Input, srcx, srcy, x, y, -1, 1);
				auto ph = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto pi = CLR(Input, srcx, srcy, x, y, 1, 1);
				auto i4 = CLR(Input, srcx, srcy, x, y, 2, 1);

				auto g5 = CLR(Input, srcx, srcy, x, y, -1, 2);
				auto h5 = CLR(Input, srcx, srcy, x, y, 0, 2);
				auto i5 = CLR(Input, srcx, srcy, x, y, 1, 2);

				int P[5];

				P[1] = P[2] = P[3] = P[4] = pe;

				_Kernel2Xv5(pe, pi, ph, pf, pg, pc, pd, pb, f4, i4, h5, i5, &P[2], &P[3], &P[4], AllowAlphaBlending);
				_Kernel2Xv5(pe, pc, pf, pb, pi, pa, ph, pd, b1, c1, f4, c4, &P[1], &P[4], &P[2], AllowAlphaBlending);
				_Kernel2Xv5(pe, pa, pb, pd, pc, pg, pf, ph, d0, a0, b1, a1, &P[3], &P[2], &P[1], AllowAlphaBlending);
				_Kernel2Xv5(pe, pg, pd, ph, pa, pi, pb, pf, h5, g5, d0, g0, &P[4], &P[1], &P[3], AllowAlphaBlending);

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void XBR3X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 3, 3);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				/*
				A1 B1 C1
				A0 PA PB PC C4    P1 P2 P3
				D0 PD PE PF F4 => P4 P5 P6
				G0 PG PH PI I4    P7 P8 P9
				G5 H5 I5
				*/

				auto a1 = CLR(Input, srcx, srcy, x, y, -1, -2);
				auto b1 = CLR(Input, srcx, srcy, x, y, 0, -2);
				auto c1 = CLR(Input, srcx, srcy, x, y, 1, -2);

				auto a0 = CLR(Input, srcx, srcy, x, y, -2, -1);
				auto pa = CLR(Input, srcx, srcy, x, y, -1, -1);
				auto pb = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto pc = CLR(Input, srcx, srcy, x, y, 1, -1);
				auto c4 = CLR(Input, srcx, srcy, x, y, 2, -1);

				auto d0 = CLR(Input, srcx, srcy, x, y, -2, 0);
				auto pd = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto pe = CLR(Input, srcx, srcy, x, y, 0, 0);
				auto pf = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto f4 = CLR(Input, srcx, srcy, x, y, 2, 0);

				auto g0 = CLR(Input, srcx, srcy, x, y, -2, 1);
				auto pg = CLR(Input, srcx, srcy, x, y, -1, 1);
				auto ph = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto pi = CLR(Input, srcx, srcy, x, y, 1, 1);
				auto i4 = CLR(Input, srcx, srcy, x, y, 2, 1);

				auto g5 = CLR(Input, srcx, srcy, x, y, -1, 2);
				auto h5 = CLR(Input, srcx, srcy, x, y, 0, 2);
				auto i5 = CLR(Input, srcx, srcy, x, y, 1, 2);

				int P[10];

				P[1] = P[2] = P[3] = P[4] = P[5] = P[6] = P[7] = P[8] = P[9] = pe;

				_Kernel3X(pe, pi, ph, pf, pg, pc, pd, pb, f4, i4, h5, i5, &P[3], &P[6], &P[7], &P[8], &P[9], AllowAlphaBlending, UseOriginalImplementation);
				_Kernel3X(pe, pc, pf, pb, pi, pa, ph, pd, b1, c1, f4, c4, &P[1], &P[2], &P[9], &P[6], &P[3], AllowAlphaBlending, UseOriginalImplementation);
				_Kernel3X(pe, pa, pb, pd, pc, pg, pf, ph, d0, a0, b1, a1, &P[7], &P[4], &P[3], &P[2], &P[1], AllowAlphaBlending, UseOriginalImplementation);
				_Kernel3X(pe, pg, pd, ph, pa, pi, pb, pf, h5, g5, d0, g0, &P[9], &P[8], &P[1], &P[4], &P[7], AllowAlphaBlending, UseOriginalImplementation);

				for (auto Pixel = 1; Pixel < 10; Pixel++)
				{
					Write9RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void XBR4X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 4, 4);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				/*
				A1 B1 C1          P01|P02|P03|P04
				A0 PA PB PC C4    P05|P06|P07|p08
				D0 PD PE PF F4 => ---+---+---+---
				G0 PG PH PI I4    P09|P10|P11|p12
				G5 H5 I5          P13|P14|P15|p16
				*/

				auto a1 = CLR(Input, srcx, srcy, x, y, -1, -2);
				auto b1 = CLR(Input, srcx, srcy, x, y, 0, -2);
				auto c1 = CLR(Input, srcx, srcy, x, y, 1, -2);

				auto a0 = CLR(Input, srcx, srcy, x, y, -2, -1);
				auto pa = CLR(Input, srcx, srcy, x, y, -1, -1);
				auto pb = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto pc = CLR(Input, srcx, srcy, x, y, 1, -1);
				auto c4 = CLR(Input, srcx, srcy, x, y, 2, -1);

				auto d0 = CLR(Input, srcx, srcy, x, y, -2, 0);
				auto pd = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto pe = CLR(Input, srcx, srcy, x, y, 0, 0);
				auto pf = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto f4 = CLR(Input, srcx, srcy, x, y, 2, 0);

				auto g0 = CLR(Input, srcx, srcy, x, y, -2, 1);
				auto pg = CLR(Input, srcx, srcy, x, y, -1, 1);
				auto ph = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto pi = CLR(Input, srcx, srcy, x, y, 1, 1);
				auto i4 = CLR(Input, srcx, srcy, x, y, 2, 1);

				auto g5 = CLR(Input, srcx, srcy, x, y, -1, 2);
				auto h5 = CLR(Input, srcx, srcy, x, y, 0, 2);
				auto i5 = CLR(Input, srcx, srcy, x, y, 1, 2);

				int P[17];

				P[1] = P[2] = P[3] = P[4] = P[5] = P[6] = P[7] = P[8] = pe;
				P[9] = P[10] = P[11] = P[12] = P[13] = P[14] = P[15] = P[16] = pe;

				_Kernel4Xv2(pe, pi, ph, pf, pg, pc, pd, pb, f4, i4, h5, i5, &P[16], &P[15], &P[12], &P[3], &P[8], &P[11], &P[14], &P[13], AllowAlphaBlending);
				_Kernel4Xv2(pe, pc, pf, pb, pi, pa, ph, pd, b1, c1, f4, c4, &P[3], &P[8], &P[3], &P[1], &P[2], &P[7], &P[12], &P[16], AllowAlphaBlending);
				_Kernel4Xv2(pe, pa, pb, pd, pc, pg, pf, ph, d0, a0, b1, a1, &P[1], &P[2], &P[5], &P[13], &P[9], &P[6], &P[3], &P[3], AllowAlphaBlending);
				_Kernel4Xv2(pe, pg, pd, ph, pa, pi, pb, pf, h5, g5, d0, g0, &P[13], &P[9], &P[14], &P[16], &P[15], &P[10], &P[5], &P[1], AllowAlphaBlending);

				for (auto Pixel = 1; Pixel < 17; Pixel++)
				{
					Write16RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void XBRZ2X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		auto src = (uint32_t*)malloc(srcx * srcy * sizeof(uint32_t));
		auto dst = (uint32_t*)malloc(SizeX * SizeY * sizeof(uint32_t));

		xbrz::Copy(src, Input, srcx, srcy);
		xbrz::scale(2, src, dst, srcx, srcy, xbrz::ColorFormat::ARGB, xbrz::ScalerCfg(), 0, srcy);
		xbrz::Copy(ScaledImage, dst, SizeX, SizeY);
	}

	void XBRZ3X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 3, 3);

		auto src = (uint32_t*)malloc(srcx * srcy * sizeof(uint32_t));
		auto dst = (uint32_t*)malloc(SizeX * SizeY * sizeof(uint32_t));

		xbrz::Copy(src, Input, srcx, srcy);
		xbrz::scale(3, src, dst, srcx, srcy, xbrz::ColorFormat::ARGB, xbrz::ScalerCfg(), 0, srcy);
		xbrz::Copy(ScaledImage, dst, SizeX, SizeY);
	}

	void XBRZ4X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 4, 4);

		auto src = (uint32_t*)malloc(srcx * srcy * sizeof(uint32_t));
		auto dst = (uint32_t*)malloc(SizeX * SizeY * sizeof(uint32_t));

		xbrz::Copy(src, Input, srcx, srcy);
		xbrz::scale(4, src, dst, srcx, srcy, xbrz::ColorFormat::ARGB, xbrz::ScalerCfg(), 0, srcy);
		xbrz::Copy(ScaledImage, dst, SizeX, SizeY);
	}

	void XBRZ5X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 5, 5);

		auto src = (uint32_t*)malloc(srcx * srcy * sizeof(uint32_t));
		auto dst = (uint32_t*)malloc(SizeX * SizeY * sizeof(uint32_t));

		xbrz::Copy(src, Input, srcx, srcy);
		xbrz::scale(5, src, dst, srcx, srcy, xbrz::ColorFormat::ARGB, xbrz::ScalerCfg(), 0, srcy);
		xbrz::Copy(ScaledImage, dst, SizeX, SizeY);
	}

	void XBRZ6X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 6, 6);

		auto src = (uint32_t*)malloc(srcx * srcy * sizeof(uint32_t));
		auto dst = (uint32_t*)malloc(SizeX * SizeY * sizeof(uint32_t));

		xbrz::Copy(src, Input, srcx, srcy);
		xbrz::scale(6, src, dst, srcx, srcy, xbrz::ColorFormat::ARGB, xbrz::ScalerCfg(), 0, srcy);
		xbrz::Copy(ScaledImage, dst, SizeX, SizeY);
	}

	void HQ2X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto c0 = CLR(Input, srcx, srcy, x, y, -1, -1);
				auto c1 = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto c2 = CLR(Input, srcx, srcy, x, y, 1, -1);
				auto c3 = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c4 = CLR(Input, srcx, srcy, x, y);
				auto c5 = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto c6 = CLR(Input, srcx, srcy, x, y, -1, 1);
				auto c7 = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto c8 = CLR(Input, srcx, srcy, x, y, 1, 1);

				unsigned char pattern = 0;
				if ((IsNotLike(c4, c0)))
					pattern |= 1;
				if ((IsNotLike(c4, c1)))
					pattern |= 2;
				if ((IsNotLike(c4, c2)))
					pattern |= 4;
				if ((IsNotLike(c4, c3)))
					pattern |= 8;
				if ((IsNotLike(c4, c5)))
					pattern |= 16;
				if ((IsNotLike(c4, c6)))
					pattern |= 32;
				if ((IsNotLike(c4, c7)))
					pattern |= 64;
				if ((IsNotLike(c4, c8)))
					pattern |= 128;

				int P[5];

				Hq2xKernel(pattern, c0, c1, c2, c3, c4, c5, c6, c7, c8, P);

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void HQ3X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 3, 3);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto c0 = CLR(Input, srcx, srcy, x, y, -1, -1);
				auto c1 = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto c2 = CLR(Input, srcx, srcy, x, y, 1, -1);
				auto c3 = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c4 = CLR(Input, srcx, srcy, x, y);
				auto c5 = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto c6 = CLR(Input, srcx, srcy, x, y, -1, 1);
				auto c7 = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto c8 = CLR(Input, srcx, srcy, x, y, 1, 1);

				unsigned char pattern = 0;
				if ((IsNotLike(c4, c0)))
					pattern |= 1;
				if ((IsNotLike(c4, c1)))
					pattern |= 2;
				if ((IsNotLike(c4, c2)))
					pattern |= 4;
				if ((IsNotLike(c4, c3)))
					pattern |= 8;
				if ((IsNotLike(c4, c5)))
					pattern |= 16;
				if ((IsNotLike(c4, c6)))
					pattern |= 32;
				if ((IsNotLike(c4, c7)))
					pattern |= 64;
				if ((IsNotLike(c4, c8)))
					pattern |= 128;

				int P[10];

				Hq3xKernel(pattern, c0, c1, c2, c3, c4, c5, c6, c7, c8, P);

				for (auto Pixel = 1; Pixel < 10; Pixel++)
				{
					Write9RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void HQ4X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 4, 4);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto c0 = CLR(Input, srcx, srcy, x, y, -1, -1);
				auto c1 = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto c2 = CLR(Input, srcx, srcy, x, y, 1, -1);
				auto c3 = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c4 = CLR(Input, srcx, srcy, x, y);
				auto c5 = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto c6 = CLR(Input, srcx, srcy, x, y, -1, 1);
				auto c7 = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto c8 = CLR(Input, srcx, srcy, x, y, 1, 1);

				unsigned char pattern = 0;
				if ((IsNotLike(c4, c0)))
					pattern |= 1;
				if ((IsNotLike(c4, c1)))
					pattern |= 2;
				if ((IsNotLike(c4, c2)))
					pattern |= 4;
				if ((IsNotLike(c4, c3)))
					pattern |= 8;
				if ((IsNotLike(c4, c5)))
					pattern |= 16;
				if ((IsNotLike(c4, c6)))
					pattern |= 32;
				if ((IsNotLike(c4, c7)))
					pattern |= 64;
				if ((IsNotLike(c4, c8)))
					pattern |= 128;

				int P[17];

				Hq4xKernel(pattern, c0, c1, c2, c3, c4, c5, c6, c7, c8, P);

				for (auto Pixel = 1; Pixel < 17; Pixel++)
				{
					Write16RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void SAI2X(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto c0 = CLR(Input, srcx, srcy, x, y, -1, -1);
				auto c1 = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto c2 = CLR(Input, srcx, srcy, x, y, 1, -1);
				auto d3 = CLR(Input, srcx, srcy, x, y, 2, -1);
				auto c3 = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c4 = CLR(Input, srcx, srcy, x, y);
				auto c5 = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto d4 = CLR(Input, srcx, srcy, x, y, 2, 0);
				auto c6 = CLR(Input, srcx, srcy, x, y, -1, 1);
				auto c7 = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto c8 = CLR(Input, srcx, srcy, x, y, 1, 1);
				auto d5 = CLR(Input, srcx, srcy, x, y, 2, 1);
				auto d0 = CLR(Input, srcx, srcy, x, y, -1, 2);
				auto d1 = CLR(Input, srcx, srcy, x, y, 0, 2);
				auto d2 = CLR(Input, srcx, srcy, x, y, 1, 2);

				int P[5];

				P[1] = P[2] = P[3] = P[4] = c4;

				if (IsLike(c4, c8) && IsNotLike(c5, c7)) {
					auto c48 = Interpolate(c4, c8);
					if ((IsLike(c48, c1) && IsLike(c5, d5)) || (IsLike(c48, c7) && IsLike(c48, c2) && IsNotLike(c5, c1) && IsLike(c5, d3))) {
						//nothing
					}
					else {
						P[2] = Interpolate(c48, c5);
					}

					if ((IsLike(c48, c3) && IsLike(c7, d2)) || (IsLike(c48, c5) && IsLike(c48, c6) && IsNotLike(c3, c7) && IsLike(c7, d0))) {
						//nothing
					}
					else {
						P[3] = Interpolate(c48, c7);
					}
				}
				else if (IsLike(c5, c7) && IsNotLike(c4, c8)) {
					auto c57 = Interpolate(c5, c7);
					if ((IsLike(c57, c2) && IsLike(c4, c6)) || (IsLike(c57, c1) && IsLike(c57, c8) && IsNotLike(c4, c2) && IsLike(c4, c0))) {
						P[2] = c57;
					}
					else {
						P[2] = Interpolate(c4, c57);
					}

					if ((IsLike(c57, c6) && IsLike(c4, c2)) || (IsLike(c57, c3) && IsLike(c57, c8) && IsNotLike(c4, c6) && IsLike(c4, c0))) {
						P[3] = c57;
					}
					else {
						P[3] = Interpolate(c4, c57);
					}
					P[4] = c57;
				}
				else if (IsLike(c4, c8) && IsLike(c5, c7)) {
					auto c48 = Interpolate(c4, c8);
					auto c57 = Interpolate(c5, c7);
					if (IsNotLike(c48, c57)) {
						auto conc2D = 0;
						conc2D += _Conc2D(c48, c57, c3, c1);
						conc2D -= _Conc2D(c57, c48, d4, c2);
						conc2D -= _Conc2D(c57, c48, c6, d1);
						conc2D += _Conc2D(c48, c57, d5, d2);

						if (conc2D < 0) {
							P[4] = c57;
						}
						else if (conc2D == 0) {
							P[4] = Interpolate(c48, c57);
						}
						P[3] = Interpolate(c48, c57);
						P[2] = Interpolate(c48, c57);
					}
				}
				else {
					P[4] = Interpolate4(c4, c5, c7, c8);

					if (IsLike(c4, c7) && IsLike(c4, c2) && IsNotLike(c5, c1) && IsLike(c5, d3)) {
						//nothing
					}
					else if (IsLike(c5, c1) && IsLike(c5, c8) && IsNotLike(c4, c2) && IsLike(c4, c0)) {
						P[2] = Interpolate(c5, c1, c8);
					}
					else {
						P[2] = Interpolate(c4, c5);
					}

					if (IsLike(c4, c5) && IsLike(c4, c6) && IsNotLike(c3, c7) && IsLike(c7, d0)) {
						//nothing
					}
					else if (IsLike(c7, c3) && IsLike(c7, c8) && IsNotLike(c4, c6) && IsLike(c4, c0)) {
						P[3] = Interpolate(c7, c3, c8);
					}
					else {
						P[3] = Interpolate(c4, c7);
					}
				}

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void SuperSAI(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto c0 = CLR(Input, srcx, srcy, x, y, -1, -1);
				auto c1 = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto c2 = CLR(Input, srcx, srcy, x, y, 1, -1);
				auto d3 = CLR(Input, srcx, srcy, x, y, 2, -1);
				auto c3 = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c4 = CLR(Input, srcx, srcy, x, y);
				auto c5 = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto d4 = CLR(Input, srcx, srcy, x, y, 2, 0);
				auto c6 = CLR(Input, srcx, srcy, x, y, -1, 1);
				auto c7 = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto c8 = CLR(Input, srcx, srcy, x, y, 1, 1);
				auto d5 = CLR(Input, srcx, srcy, x, y, 2, 1);
				auto d0 = CLR(Input, srcx, srcy, x, y, -1, 2);
				auto d1 = CLR(Input, srcx, srcy, x, y, 0, 2);
				auto d2 = CLR(Input, srcx, srcy, x, y, 1, 2);
				auto d6 = CLR(Input, srcx, srcy, x, y, 2, 2);

				int P[5];

				P[1] = P[2] = P[3] = P[4] = c4;

				if (IsLike(c7, c5) && IsNotLike(c4, c8)) {
					auto c57 = Interpolate(c7, c5);
					P[4] = c57;
					P[2] = c57;
				}
				else if (IsLike(c4, c8) && IsNotLike(c7, c5)) {
					//nothing
				}
				else if (IsLike(c4, c8) && IsLike(c7, c5)) {
					auto c57 = Interpolate(c7, c5);
					auto c48 = Interpolate(c4, c8);
					auto conc2D = 0;
					conc2D += _Conc2D(c57, c48, c6, d1);
					conc2D += _Conc2D(c57, c48, c3, c1);
					conc2D += _Conc2D(c57, c48, d2, d5);
					conc2D += _Conc2D(c57, c48, c2, d4);

					if (conc2D > 0) {
						P[4] = c57;
						P[2] = c57;
					}
					else if (conc2D == 0) {
						P[4] = Interpolate(c48, c57);
						P[2] = Interpolate(c48, c57);
					}
				}
				else {
					if (IsLike(c8, c5) && IsLike(c8, d1) && IsNotLike(c7, d2) && IsNotLike(c8, d0)) {
						P[4] = Interpolate(Interpolate(c8, c5, d1), c7, 3, 1);
					}
					else if (IsLike(c7, c4) && IsLike(c7, d2) && IsNotLike(c7, d6) && IsNotLike(c8, d1)) {
						P[4] = Interpolate(Interpolate(c7, c4, d2), c8, 3, 1);
					}
					else {
						P[4] = Interpolate(c7, c8);
					}
					if (IsLike(c5, c8) && IsLike(c5, c1) && IsNotLike(c5, c0) && IsNotLike(c4, c2)) {
						P[2] = Interpolate(Interpolate(c5, c8, c1), c4, 3, 1);
					}
					else if (IsLike(c4, c7) && IsLike(c4, c2) && IsNotLike(c5, c1) && IsNotLike(c4, d3)) {
						P[2] = Interpolate(Interpolate(c4, c7, c2), c5, 3, 1);
					}
					else {
						P[2] = Interpolate(c4, c5);
					}
				}
				if (IsLike(c4, c8) && IsLike(c4, c3) && IsNotLike(c7, c5) && IsNotLike(c4, d2)) {
					P[3] = Interpolate(c7, Interpolate(c4, c8, c3));
				}
				else if (IsLike(c4, c6) && IsLike(c4, c5) && IsNotLike(c7, c3) && IsNotLike(c4, d0)) {
					P[3] = Interpolate(c7, Interpolate(c4, c6, c5));
				}
				else {
					P[3] = c7;
				}

				if (IsLike(c7, c5) && IsLike(c7, c6) && IsNotLike(c4, c8) && IsNotLike(c7, c2)) {
					P[1] = Interpolate(Interpolate(c7, c5, c6), c4);
				}
				else if (IsLike(c7, c3) && IsLike(c7, c8) && IsNotLike(c4, c6) && IsNotLike(c7, c0)) {
					P[1] = Interpolate(Interpolate(c7, c3, c8), c4);
				}

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void SuperEagle(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto c1 = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto c2 = CLR(Input, srcx, srcy, x, y, 1, -1);
				auto c3 = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto c4 = CLR(Input, srcx, srcy, x, y);
				auto c5 = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto d4 = CLR(Input, srcx, srcy, x, y, 2, 0);
				auto c6 = CLR(Input, srcx, srcy, x, y, -1, 1);
				auto c7 = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto c8 = CLR(Input, srcx, srcy, x, y, 1, 1);
				auto d5 = CLR(Input, srcx, srcy, x, y, 2, 1);
				auto d1 = CLR(Input, srcx, srcy, x, y, 0, 2);
				auto d2 = CLR(Input, srcx, srcy, x, y, 1, 2);

				int P[5];

				P[1] = P[2] = P[3] = P[4] = c4;

				if (IsLike(c4, c8)) {
					auto c48 = Interpolate(c4, c8);
					if (IsLike(c7, c5)) {
						auto c57 = Interpolate(c5, c7);
						auto conc2D = 0;
						conc2D += _Conc2D(c57, c48, c6, d1);
						conc2D += _Conc2D(c57, c48, c3, c1);
						conc2D += _Conc2D(c57, c48, d2, d5);
						conc2D += _Conc2D(c57, c48, c2, d4);

						if (conc2D > 0) {
							P[3] = c57;
							P[2] = c57;
							P[4] = Interpolate(c48, c57);
							P[1] = Interpolate(c48, c57);
						}
						else if (conc2D < 0) {
							P[3] = Interpolate(c48, c57);
							P[2] = Interpolate(c48, c57);
						}
						else {
							P[3] = c57;
							P[2] = c57;
						}
					}
					else {
						if (IsLike(c48, c1) && IsLike(c48, d5))
							P[2] = Interpolate(Interpolate(c48, c1, d5), c5, 3, 1);
						else if (IsLike(c48, c1))
							P[2] = Interpolate(Interpolate(c48, c1), c5, 3, 1);
						else if (IsLike(c48, d5))
							P[2] = Interpolate(Interpolate(c48, d5), c5, 3, 1);
						else
							P[2] = Interpolate(c48, c5);

						if (IsLike(c48, d2) && IsLike(c48, c3))
							P[3] = Interpolate(Interpolate(c48, d2, c3), c7, 3, 1);
						else if (IsLike(c48, d2))
							P[3] = Interpolate(Interpolate(c48, d2), c7, 3, 1);
						else if (IsLike(c48, c3))
							P[3] = Interpolate(Interpolate(c48, c3), c7, 3, 1);
						else
							P[3] = Interpolate(c48, c7);

					}
				}
				else {
					if (IsLike(c7, c5)) {
						auto c57 = Interpolate(c5, c7);
						P[2] = c57;
						P[3] = c57;

						if (IsLike(c57, c6) && IsLike(c57, c2))
							P[1] = Interpolate(Interpolate(c57, c6, c2), c4, 3, 1);
						else if (IsLike(c57, c6))
							P[1] = Interpolate(Interpolate(c57, c6), c4, 3, 1);
						else if (IsLike(c57, c2))
							P[1] = Interpolate(Interpolate(c57, c2), c4, 3, 1);
						else
							P[1] = Interpolate(c57, c4);

						if (IsLike(c57, d4) && IsLike(c57, d1))
							P[4] = Interpolate(Interpolate(c57, d4, d1), c8, 3, 1);
						else if (IsLike(c57, d4))
							P[4] = Interpolate(Interpolate(c57, d4), c8, 3, 1);
						else if (IsLike(c57, d1))
							P[4] = Interpolate(Interpolate(c57, d1), c8, 3, 1);
						else
							P[4] = Interpolate(c57, c8);
					}
					else {
						P[4] = Interpolate(c8, c7, c5, 6, 1, 1);
						P[1] = Interpolate(c4, c7, c5, 6, 1, 1);
						P[3] = Interpolate(c7, c4, c8, 6, 1, 1);
						P[2] = Interpolate(c5, c4, c8, 6, 1, 1);
					}
				}

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void ReverseAA(unsigned char*& Input, int srcx, int srcy)
	{
		Init(srcx, srcy, 2, 2);

		for (auto y = 0; y < srcy; y++)
		{
			for (auto x = 0; x < srcx; x++)
			{
				auto b1 = CLR(Input, srcx, srcy, x, y, 0, -2);
				auto b = CLR(Input, srcx, srcy, x, y, 0, -1);
				auto d = CLR(Input, srcx, srcy, x, y, -1, 0);
				auto e = CLR(Input, srcx, srcy, x, y);
				auto f = CLR(Input, srcx, srcy, x, y, 1, 0);
				auto h = CLR(Input, srcx, srcy, x, y, 0, 1);
				auto h5 = CLR(Input, srcx, srcy, x, y, 0, 2);
				auto d0 = CLR(Input, srcx, srcy, x, y, -2, 0);
				auto f4 = CLR(Input, srcx, srcy, x, y, 2, 0);

				int rr, rg, rb, ra = 0;
				int gr, gg, gb, ga = 0;
				int br, bg, bb, ba = 0;

				_ReverseAntiAlias(Red(b1), Red(b), Red(d), Red(e), Red(f), Red(h), Red(h5), Red(d0), Red(f4), &rr, &rg, &rb, &ra);
				_ReverseAntiAlias(Green(b1), Green(b), Green(d), Green(e), Green(f), Green(h), Green(h5), Green(d0), Green(f4), &gr, &gg, &gb, &ga);
				_ReverseAntiAlias(Blue(b1), Blue(b), Blue(d), Blue(e), Blue(f), Blue(h), Blue(h5), Blue(d0), Blue(f4), &br, &bg, &bb, &ba);

				int P[5];

				P[1] = RGBINT(_FullClamp(rr), _FullClamp(gr), _FullClamp(br));
				P[2] = RGBINT(_FullClamp(rg), _FullClamp(gg), _FullClamp(bg));
				P[3] = RGBINT(_FullClamp(rb), _FullClamp(gb), _FullClamp(bb));
				P[4] = RGBINT(_FullClamp(ra), _FullClamp(ga), _FullClamp(ba));

				for (auto Pixel = 1; Pixel < 5; Pixel++)
				{
					Write4RGB(ScaledImage, srcx, srcy, x, y, Pixel, P[Pixel]);
				}
			}
		}
	}

	void Kuwahara(unsigned char*& Input, int srcx, int srcy, int win)
	{
		Init(srcx, srcy, 1, 1);

		_Kuwahara(ScaledImage, Input, srcx, srcy, win);
	}
};
