#define _USE_MATH_DEFINES
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "lodepng.h"
#include "Parameters.h"
#include "PixelScaler.h"
#include "Profiler.h"

unsigned char* loadpng(char* filename, int* xdim, int* ydim)
{
	unsigned error;
	unsigned char* image;
	unsigned w, h;

	// load PNG
	error = lodepng_decode24_file(&image, &w, &h, filename);

	// exit on error
	if (error)
	{
		fprintf(stderr, "decoder error %u: %s\n", error, lodepng_error_text(error));

		exit(1);
	}

	*xdim = w;
	*ydim = h;

	fprintf(stderr, "Image: %s loaded, size = (%d x %d)\n", filename, w, h);

	return image;
}

unsigned char* loadimage(char* filename, int* xdim, int* ydim)
{
	auto Channels = 3;

	auto img = cv::imread(filename, cv::IMREAD_COLOR);

	if (img.empty()) // Check for invalid input
	{
		fprintf(stderr, "Error on loading image: %s\n", filename);

		exit(1);
	}

	fprintf(stderr, "loaded %d x %d image\n", img.cols, img.rows);

	auto *image = (unsigned char*)malloc(img.cols * img.rows * Channels);

	for (auto y = 0; y < img.rows; y++)
	{
		for (auto x = 0; x < img.cols; x++)
		{
			auto b = img.at<cv::Vec3b>(y, x)[0];
			auto g = img.at<cv::Vec3b>(y, x)[1];
			auto r = img.at<cv::Vec3b>(y, x)[2];

			image[(y * img.cols + x) * Channels] = r;
			image[(y * img.cols + x) * Channels + 1] = g;
			image[(y * img.cols + x) * Channels + 2] = b;
		}
	}

	*xdim = img.cols;
	*ydim = img.rows;

	img.release();

	return image;
}

void savepng(const char* filename, unsigned char*& buffer, int xdim, int ydim)
{
	auto error = lodepng_encode24_file(filename, buffer, xdim, ydim);

	if (error)
	{
		fprintf(stderr, "error %u: %s\n", error, lodepng_error_text(error));
	}
}

void ParseInt(std::string arg, const char* str, const char* var, int& dst)
{
	auto len = strlen(str);

	if (len > 0)
	{
		if (!arg.compare(0, len, str) && arg.length() > len)
		{
			try
			{
				auto val = stoi(arg.substr(len));

				fprintf(stderr, "... %s = %d\n", var, val);

				dst = val;
			}
			catch (const std::invalid_argument& ia)
			{
				fprintf(stderr, "... %s = NaN %s\n", var, ia.what());
				exit(1);
			}
		}
	}
}

void ParseBool(std::string arg, const char* str, const char* var, bool& dst)
{
	auto len = strlen(str);

	if (len > 0)
	{
		if (!arg.compare(0, len, str) && arg.length() > len)
		{
			try
			{
				auto val = stoi(arg.substr(len));

				fprintf(stderr, "... %s = %s\n", var, val > 0 ? "on" : "off");

				dst = val > 0;
			}
			catch (const std::invalid_argument& ia)
			{
				fprintf(stderr, "... %s = Nan %s\n", var, ia.what());

				exit(1);
			}
		}
	}
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "To use:\n\n%s [parameters...]\n", argv[0]);

		exit(1);
	}

	Parameters parameters;

	char InputFile[200];
	char OutputFile[200];

	InputFile[0] = '\0';
	OutputFile[0] = '\0';

	for (int i = 0; i < argc; i++)
	{
		std::string arg = argv[i];
		std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);

		if (!arg.compare("/help"))
		{
			fprintf(stderr, "\nAvailable parameters:\n");
			fprintf(stderr, "=====================\n\n");
			fprintf(stderr, "/output=[output.png] specify output image png file\n\n");
			fprintf(stderr, "/input=[input.png] specify input image png file\n\n");
			fprintf(stderr, "/filters show available scaling filters\n\n");
			fprintf(stderr, "/scaling=[epx/scale2x, use /filters option for list of filters] specify type of pixel scaling to apply\n\n");
			fprintf(stderr, "/magnification=[magnification, e.g. 2] specify magnification\n\n");
			fprintf(stderr, "/threshold=[0, 1 default: 0] enable / disable threshold when comparing pixels\n\n");
			exit(0);
		}

		if (!arg.compare("/filters"))
		{
			fprintf(stderr, "\nAvailable Filters:\n");
			fprintf(stderr, "=====================\n\n");
			fprintf(stderr, "bilnear, nearest\n");
			fprintf(stderr, "epx, epxb, epxc, epx3\n");
			fprintf(stderr, "scale2x, scale3x, scale4x\n");
			fprintf(stderr, "magnify, magnify2x, magnify3x, magnify4x\n");
			fprintf(stderr, "2xsai, epxmag2, mag2epx\n");
			fprintf(stderr, "eagle2x, eagle3x, eagle3xb\n");
			fprintf(stderr, "supereagle, supersai, reverseaa\n");
			fprintf(stderr, "des2x, super2x, ultra2x\n");
			fprintf(stderr, "hq2x, hq3x, hq4x\n");
			fprintf(stderr, "lq2x, lq3x, lq4x\n");
			fprintf(stderr, "tv2x, tv3x, horiz2x, horiz3x, vertscan2x, rgb2x, rgb3x\n");
			fprintf(stderr, "advinterp2x, advinterp3x, 2xscl\n");
			fprintf(stderr, "xbr2x, xbr3x, xbr4x\n");
			fprintf(stderr, "xbrz2x, xbrz3x, xbrz4x, xbrz5x, xbrz6x\n");
			fprintf(stderr, "kuwahara,kuwahara7,kuwahara9,kuwahara11\n");
			fprintf(stderr, "\n");

			exit(0);
		}

		if (!arg.compare(0, 9, "/scaling=") && arg.length() > 9)
		{
			auto type = arg.substr(9);

			if (!type.compare("epx") || !type.compare("scale2x"))
			{
				fprintf(stderr, "Scaling: EPX / Scale2X\n");
				parameters.EPX = true;
			}

			if (!type.compare("epxb"))
			{
				fprintf(stderr, "Scaling: EPX ver. B\n");
				parameters.EPXB = true;
			}

			if (!type.compare("epxc"))
			{
				fprintf(stderr, "Scaling: EPX ver. C\n");
				parameters.EPXC = true;
			}

			if (!type.compare("epx3"))
			{
				fprintf(stderr, "Scaling: EPX 3X\n");
				parameters.EPX3 = true;
			}

			if (!type.compare("scale4x"))
			{
				fprintf(stderr, "Scaling: Scale4X\n");
				parameters.Scale4X = true;
			}

			if (!type.compare("scale3x"))
			{
				fprintf(stderr, "Scaling: Scale3X\n");
				parameters.Scale3X = true;
			}

			if (!type.compare("magnify"))
			{
				fprintf(stderr, "Scaling: Magnify\n");
				parameters.Magnify = true;
			}

			if (!type.compare("magnify2x"))
			{
				fprintf(stderr, "Scaling: Magnify 2X\n");
				parameters.Magnify = true;
				parameters.Magnification = 2;
			}

			if (!type.compare("magnify3x"))
			{
				fprintf(stderr, "Scaling: Magnify 3X\n");
				parameters.Magnify = true;
				parameters.Magnification = 3;
			}

			if (!type.compare("magnify4x"))
			{
				fprintf(stderr, "Scaling: Magnify 4X\n");
				parameters.Magnify = true;
				parameters.Magnification = 4;
			}

			if (!type.compare("mag2epx"))
			{
				fprintf(stderr, "Scaling: Magnify x 2/EPX\n");
				parameters.Mag2EPX = true;
			}

			if (!type.compare("epxmag2"))
			{
				fprintf(stderr, "Scaling: EPX/Magnify x 2\n");
				parameters.EPXMag2 = true;
			}

			if (!type.compare("nearest"))
			{
				fprintf(stderr, "Scaling: Nearest-neighbor\n");
				parameters.Nearest = true;
			}

			if (!type.compare("bilinear"))
			{
				fprintf(stderr, "Scaling: Bilinear\n");
				parameters.Bilinear = true;
			}

			if (!type.compare("lq2x"))
			{
				fprintf(stderr, "Scaling: LQ2X\n");
				parameters.LQ2X = true;
			}

			if (!type.compare("lq3x"))
			{
				fprintf(stderr, "Scaling: LQ3X\n");
				parameters.LQ3X = true;
			}

			if (!type.compare("lq4x"))
			{
				fprintf(stderr, "Scaling: LQ4X\n");
				parameters.LQ4X = true;
			}

			if (!type.compare("eagle2x"))
			{
				fprintf(stderr, "Scaling: Eagle2X\n");
				parameters.Eagle2X = true;
			}

			if (!type.compare("eagle3x"))
			{
				fprintf(stderr, "Scaling: Eagle3X\n");
				parameters.Eagle3X = true;
			}

			if (!type.compare("eagle3xb"))
			{
				fprintf(stderr, "Scaling: Eagle3XB\n");
				parameters.Eagle3XB = true;
			}

			if (!type.compare("supereagle"))
			{
				fprintf(stderr, "Scaling: Super Eagle (2X)\n");
				parameters.SuperEagle = true;
			}

			if (!type.compare("super2x"))
			{
				fprintf(stderr, "Scaling: Super2X\n");
				parameters.Super2X = true;
			}

			if (!type.compare("ultra2x"))
			{
				fprintf(stderr, "Scaling: Ultra2X\n");
				parameters.Ultra2X = true;
			}

			if (!type.compare("des2x"))
			{
				fprintf(stderr, "Scaling: DES2X\n");
				parameters.DES2X = true;
			}

			if (!type.compare("tv2x"))
			{
				fprintf(stderr, "Scaling: TV2X\n");
				parameters.TV2X = true;
			}

			if (!type.compare("tv3x"))
			{
				fprintf(stderr, "Scaling: TV3X\n");
				parameters.TV3X = true;
			}

			if (!type.compare("rgb2x"))
			{
				fprintf(stderr, "Scaling: RGB2X\n");
				parameters.RGB2X = true;
			}

			if (!type.compare("rgb3x"))
			{
				fprintf(stderr, "Scaling: RGB3X\n");
				parameters.RGB3X = true;
			}

			if (!type.compare("horiz2x"))
			{
				fprintf(stderr, "Scaling: Horizontal Scanlines (2X)\n");
				parameters.Horiz2X = true;
			}

			if (!type.compare("horiz3x"))
			{
				fprintf(stderr, "Scaling: Horizontal Scanlines (3X)\n");
				parameters.Horiz3X = true;
			}

			if (!type.compare("advinterp2x"))
			{
				fprintf(stderr, "Scaling: AdvanceMame Interpolation (2X)\n");
				parameters.AdvInterp2X = true;
			}

			if (!type.compare("advinterp3x"))
			{
				fprintf(stderr, "Scaling: AdvanceMame Interpolation (3X)\n");
				parameters.AdvInterp3X = true;
			}

			if (!type.compare("vertscan2x"))
			{
				fprintf(stderr, "Scaling: Vertical Scanlines (2X)\n");
				parameters.VertScan2X = true;
			}

			if (!type.compare("2xscl"))
			{
				fprintf(stderr, "Scaling: SNES 2XSCL (2X)\n");
				parameters.SCL2X = true;
			}

			if (!type.compare("xbr2x"))
			{
				fprintf(stderr, "Scaling: XBR (2X)\n");
				parameters.XBR2X = true;
			}

			if (!type.compare("xbr3x"))
			{
				fprintf(stderr, "Scaling: XBR (3X)\n");
				parameters.XBR3X = true;
			}

			if (!type.compare("xbr4x"))
			{
				fprintf(stderr, "Scaling: XBR (4X)\n");
				parameters.XBR4X = true;
			}

			if (!type.compare("xbrz2x"))
			{
				fprintf(stderr, "Scaling: XBRZ (2X)\n");
				parameters.XBRZ2X = true;
			}

			if (!type.compare("xbrz3x"))
			{
				fprintf(stderr, "Scaling: XBRZ (3X)\n");
				parameters.XBRZ3X = true;
			}

			if (!type.compare("xbrz4x"))
			{
				fprintf(stderr, "Scaling: XBRZ (4X)\n");
				parameters.XBRZ4X = true;
			}

			if (!type.compare("xbrz5x"))
			{
				fprintf(stderr, "Scaling: XBRZ (5X)\n");
				parameters.XBRZ5X = true;
			}

			if (!type.compare("xbrz6x"))
			{
				fprintf(stderr, "Scaling: XBRZ (6X)\n");
				parameters.XBRZ6X = true;
			}

			if (!type.compare("hq2x"))
			{
				fprintf(stderr, "Scaling: HQnX (2X)\n");
				parameters.HQ2X = true;
			}

			if (!type.compare("hq3x"))
			{
				fprintf(stderr, "Scaling: HQnX (3X)\n");
				parameters.HQ3X = true;
			}

			if (!type.compare("hq4x"))
			{
				fprintf(stderr, "Scaling: HQnX (4X)\n");
				parameters.HQ4X = true;
			}

			if (!type.compare("2xsai"))
			{
				fprintf(stderr, "Scaling: 2X SAI\n");
				parameters.SAI2X = true;
			}

			if (!type.compare("supersai"))
			{
				fprintf(stderr, "Scaling: Super SAI (2X)\n");
				parameters.SuperSAI = true;
			}

			if (!type.compare("reverseaa"))
			{
				fprintf(stderr, "Scaling: Reverse Anti-Aliasing (2X)\n");
				parameters.ReverseAA = true;
			}

			if (!type.compare("kuwahara"))
			{
				fprintf(stderr, "Filtering: Kuwahara Smoothing and Edge-preserving filter (1X)\n");
				parameters.Kuwahara = true;
				parameters.Window = 5;
			}

			if (!type.compare("kuwahara7"))
			{
				fprintf(stderr, "Filtering: Kuwahara Smoothing and Edge-preserving filter (1X) using a 7x7 window\n");
				parameters.Kuwahara = true;
				parameters.Window = 7;
			}

			if (!type.compare("kuwahara9"))
			{
				fprintf(stderr, "Filtering: Kuwahara Smoothing and Edge-preserving filter (1X) using a 9x9 window\n");
				parameters.Kuwahara = true;
				parameters.Window = 9;
			}

			if (!type.compare("kuwahara11"))
			{
				fprintf(stderr, "Filtering: Kuwahara Smoothing and Edge-preserving filter (1X) using a 11x11 window\n");
				parameters.Kuwahara = true;
				parameters.Window = 11;
			}
		}

		ParseInt(arg, "/magnification=", "Magnification", parameters.Magnification);
		ParseBool(arg, "/threshold=", "Threshold", parameters.Threshold);

		if (!arg.compare(0, 7, "/input=") && arg.length() > 7)
		{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
			strncpy_s(InputFile, &argv[i][7], sizeof(InputFile));
#else
			strncpy(InputFile, &argv[i][7], sizeof(InputFile));
#endif

			fprintf(stderr, "Input PNG File: %s\n", InputFile);
		}

		if (!arg.compare(0, 8, "/output=") && arg.length() > 8)
		{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
			strncpy_s(OutputFile, &argv[i][8], sizeof(OutputFile));
#else
			strncpy(OutputFile, &argv[i][8], sizeof(OutputFile));
#endif

			fprintf(stderr, "Output PNG File: %s\n", OutputFile);
		}
	}

	PixelScaler scaler;

	scaler.SetThreshold(parameters.Threshold);

	int srcx = 0;
	int srcy = 0;

	if (strlen(InputFile) < 5)
	{
		fprintf(stderr, "%s: error: no input file\n", argv[0]);
		exit(1);
	}

	auto png = loadimage(InputFile, &srcx, &srcy);

	if (parameters.EPX)
	{
		scaler.EPX(png, srcx, srcy);
	}

	if (parameters.EPXB)
	{
		scaler.EPXB(png, srcx, srcy);
	}

	if (parameters.EPXC)
	{
		scaler.EPXC(png, srcx, srcy);
	}

	if (parameters.EPX3)
	{
		scaler.EPX3(png, srcx, srcy);
	}

	if (parameters.Scale4X)
	{
		scaler.Scale4X(png, srcx, srcy);
	}

	if (parameters.Scale3X)
	{
		scaler.Scale3X(png, srcx, srcy);
	}

	if (parameters.Magnify && parameters.Magnification > 0)
	{
		scaler.Magnify(png, srcx, srcy, parameters.Magnification);
	}

	if (parameters.Mag2EPX)
	{
		scaler.Mag2EPX(png, srcx, srcy);
	}

	if (parameters.EPXMag2)
	{
		scaler.EPXMag2(png, srcx, srcy);
	}

	if (parameters.Nearest)
	{
		scaler.Nearest(png, srcx, srcy);
	}

	if (parameters.Bilinear)
	{
		scaler.Bilinear(png, srcx, srcy);
	}

	if (parameters.LQ2X)
	{
		scaler.LQ2X(png, srcx, srcy);
	}

	if (parameters.LQ3X)
	{
		scaler.LQ3X(png, srcx, srcy);
	}

	if (parameters.LQ4X)
	{
		scaler.LQ4X(png, srcx, srcy);
	}

	if (parameters.Eagle2X)
	{
		scaler.Eagle2X(png, srcx, srcy);
	}

	if (parameters.Eagle3X)
	{
		scaler.Eagle3X(png, srcx, srcy);
	}

	if (parameters.Eagle3XB)
	{
		scaler.Eagle3XB(png, srcx, srcy);
	}

	if (parameters.Super2X)
	{
		scaler.Super2X(png, srcx, srcy);
	}

	if (parameters.Ultra2X)
	{
		scaler.Ultra2X(png, srcx, srcy);
	}

	if (parameters.DES2X)
	{
		scaler.DES2X(png, srcx, srcy);
	}

	if (parameters.TV2X)
	{
		scaler.TV2X(png, srcx, srcy);
	}

	if (parameters.TV3X)
	{
		scaler.TV3X(png, srcx, srcy);
	}

	if (parameters.RGB2X)
	{
		scaler.RGB2X(png, srcx, srcy);
	}

	if (parameters.RGB3X)
	{
		scaler.RGB3X(png, srcx, srcy);
	}

	if (parameters.Horiz2X)
	{
		scaler.Horiz2X(png, srcx, srcy);
	}

	if (parameters.Horiz3X)
	{
		scaler.Horiz3X(png, srcx, srcy);
	}

	if (parameters.AdvInterp2X)
	{
		scaler.AdvInterp2X(png, srcx, srcy);
	}

	if (parameters.AdvInterp3X)
	{
		scaler.AdvInterp3X(png, srcx, srcy);
	}

	if (parameters.VertScan2X)
	{
		scaler.VertScan2X(png, srcx, srcy);
	}

	if (parameters.SCL2X)
	{
		scaler.SCL2X(png, srcx, srcy);
	}

	if (parameters.XBR2X)
	{
		scaler.XBR2X(png, srcx, srcy);
	}

	if (parameters.XBR3X)
	{
		scaler.XBR3X(png, srcx, srcy);
	}

	if (parameters.XBR4X)
	{
		scaler.XBR4X(png, srcx, srcy);
	}

	if (parameters.XBRZ2X)
	{
		scaler.XBRZ2X(png, srcx, srcy);
	}

	if (parameters.XBRZ3X)
	{
		scaler.XBRZ3X(png, srcx, srcy);
	}

	if (parameters.XBRZ4X)
	{
		scaler.XBRZ4X(png, srcx, srcy);
	}

	if (parameters.XBRZ5X)
	{
		scaler.XBRZ5X(png, srcx, srcy);
	}

	if (parameters.XBRZ6X)
	{
		scaler.XBRZ6X(png, srcx, srcy);
	}

	if (parameters.HQ2X)
	{
		scaler.HQ2X(png, srcx, srcy);
	}

	if (parameters.HQ3X)
	{
		scaler.HQ3X(png, srcx, srcy);
	}

	if (parameters.HQ4X)
	{
		scaler.HQ4X(png, srcx, srcy);
	}

	if (parameters.SAI2X)
	{
		scaler.SAI2X(png, srcx, srcy);
	}

	if (parameters.SuperEagle)
	{
		scaler.SuperEagle(png, srcx, srcy);
	}

	if (parameters.SuperSAI)
	{
		scaler.SuperSAI(png, srcx, srcy);
	}

	if (parameters.ReverseAA)
	{
		scaler.ReverseAA(png, srcx, srcy);
	}

	if (parameters.Kuwahara)
	{
		scaler.Kuwahara(png, srcx, srcy, parameters.Window);
	}

	if (strlen(OutputFile) > 0)
	{
		savepng(OutputFile, scaler.ScaledImage, scaler.SizeX, scaler.SizeY);
	}

	scaler.Release();

	free(png);

	return 0;
}
