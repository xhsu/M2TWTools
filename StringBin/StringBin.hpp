#pragma once

#include <experimental/generator>

#include <stdint.h>
#include <stdio.h>

#include <tuple>

using std::experimental::generator;
using std::tuple;

inline generator<tuple<char16_t const *, std::int16_t, char16_t const *, std::int16_t>> ParseStringsBin(FILE *f) noexcept
{
	fseek(f, 0, SEEK_SET);

	std::int16_t iStyle1 = 0, iStyle2 = 0;
	fread(&iStyle1, sizeof(iStyle1), 1, f);
	fread(&iStyle2, sizeof(iStyle2), 1, f);

	if (iStyle1 != 2 || iStyle2 != 2048)
	{
		co_yield std::make_tuple(nullptr, iStyle1, nullptr, iStyle2);
		co_return;	// WHY WHY WHY???? WHY CAN'T I JUST co_return A TUPLE?!
	}

	std::int32_t iCount = 0;
	fread(&iCount, sizeof(iCount), 1, f);

	char16_t *pszKey = nullptr, *pszValue = nullptr;
	std::int16_t iKeyLength = 0, iValueLength = 0;

	for (int i = 0; i < iCount; ++i)
	{
		iKeyLength = 0;
		fread(&iKeyLength, sizeof(iKeyLength), 1, f);

		pszKey = (char16_t *)realloc(pszKey, (iKeyLength + 1) * sizeof(char16_t));
		pszKey[iKeyLength] = u'\0';
		fread(pszKey, sizeof(char16_t), iKeyLength, f);

		iValueLength = 0;
		fread(&iValueLength, sizeof(iValueLength), 1, f);

		pszValue = (char16_t *)realloc(pszValue, (iValueLength + 1) * sizeof(char16_t));
		pszValue[iValueLength] = u'\0';
		fread(pszValue, sizeof(char16_t), iValueLength, f);

		co_yield std::make_tuple(pszKey, iKeyLength, pszValue, iValueLength);
	}

	free(pszKey);
	free(pszValue);

	co_return;
}
