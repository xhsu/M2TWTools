module;

#include <format>
#include <fstream>
#include <iostream>
#include <list>
#include <string>

export module descr_mercenaries;

import UtlConcepts;
import UtlString;

using namespace std::string_literals;
using namespace std::string_view_literals;

using std::list;
using std::string;

using uint16 = unsigned __int16;

export struct Mercenary_t
{
	string m_szUnit;
	uint16 m_iExp{ 0 };
	uint16 m_iCost{ 0 };
	float m_flReplenishMin{ 0 };
	float m_flReplenishMax{ 0 };
	uint16 m_iMax{ 0 };
	uint16 m_iInitial{ 0 };

	// Optionals
	bool m_bCrusading{ false };
	list<string> m_rgszReligions;
	list<string> m_rgszEvents;
	uint16 m_iStartYear{ 0 };
	uint16 m_iEndYear{ 0 };

	Mercenary_t(void) noexcept {}
	explicit Mercenary_t(const string& sz) noexcept { Parse(sz); }
	virtual ~Mercenary_t(void) noexcept {}

	void Parse(const string& sz) noexcept
	{
		char* pBuf = _strdup(sz.c_str());
		for (char* p = strtok(pBuf, " \f\n\r\t\v"); p != nullptr; p = strtok(nullptr, " \f\n\r\t\v"))
		{
			if (!strlen(p))
				continue;

			if (!_stricmp(p, "unit"))
			{
				p = strtok(nullptr, " \f\n\r\t\v");

				while (p != nullptr && _stricmp(p, "exp"))
				{
					m_szUnit += p + " "s;
					p = strtok(nullptr, " \f\n\r\t\v");
				}

				m_szUnit.pop_back();	// Remove the last space.
				if (m_szUnit.back() == ',')
					m_szUnit.pop_back();

				// Handle exp as well.
				if (!_stricmp(p, "exp"))
				{
					p = strtok(nullptr, " \f\n\r\t\v");
					m_iExp = UTIL_StrToNum<uint16>(p);
				}
			}
			else if (!_stricmp(p, "cost"))
			{
				p = strtok(nullptr, " \f\n\r\t\v");
				m_iCost = UTIL_StrToNum<uint16>(p);
			}
			else if (!_stricmp(p, "replenish"))
			{
				p = strtok(nullptr, " \f\n\r\t\v");
				m_flReplenishMin = UTIL_StrToNum<float>(p);
				p = strtok(nullptr, " \f\n\r\t\v");	// the dashline '-' between two numbers.
				p = strtok(nullptr, " \f\n\r\t\v");
				m_flReplenishMax = UTIL_StrToNum<float>(p);
			}
			else if (!_stricmp(p, "max"))
			{
				p = strtok(nullptr, " \f\n\r\t\v");
				m_iMax = UTIL_StrToNum<uint16>(p);
			}
			else if (!_stricmp(p, "initial"))
			{
				p = strtok(nullptr, " \f\n\r\t\v");
				m_iInitial = UTIL_StrToNum<uint16>(p);
			}
			else if (!_stricmp(p, "crusading"))
			{
				m_bCrusading = true;
			}
			else if (!_stricmp(p, "religions"))
			{
				p = strtok(nullptr, " \f\n\r\t\v");

				while (p != nullptr && *p != '}')
				{
					if (*p != '{')
						m_rgszReligions.emplace_back(p);

					p = strtok(nullptr, " \f\n\r\t\v");
				}
			}
			else if (!_stricmp(p, "events"))
			{
				p = strtok(nullptr, " \f\n\r\t\v");

				while (p != nullptr && *p != '}')
				{
					if (*p != '{')
						m_rgszEvents.emplace_back(p);

					p = strtok(nullptr, " \f\n\r\t\v");
				}
			}
			else if (!_stricmp(p, "start_year"))
			{
				p = strtok(nullptr, " \f\n\r\t\v");
				m_iStartYear = UTIL_StrToNum<uint16>(p);
			}
			else if (!_stricmp(p, "end_year"))
			{
				p = strtok(nullptr, " \f\n\r\t\v");
				m_iEndYear = UTIL_StrToNum<uint16>(p);
			}
			else
			{
				std::cout << "Unknow key during parse: \"" << p << '"' << std::endl;
			}
		}
	}

	string ToString(size_t iBlockAlignedCharCount = 1) const noexcept
	{
		auto iMyLen = strlen_c("unit ") + m_szUnit.length();
		auto iIndents = 0;

		for (int iDelta = iBlockAlignedCharCount - iMyLen; iDelta > 0; iDelta -= 4)
			++iIndents;

		iIndents = std::max(1, iIndents);	// at least one.
		string ret = std::format("unit {}{}exp {} cost {} replenish {} - {} max {} initial {}", m_szUnit, string(iIndents, '\t'), m_iExp, m_iCost, m_flReplenishMin, m_flReplenishMax, m_iMax, m_iInitial);

		if (m_iStartYear)
			ret += " start_year " + std::to_string(m_iStartYear);
		if (m_iEndYear)
			ret += " end_year " + std::to_string(m_iEndYear);

		if (!m_rgszReligions.empty())
		{
			ret += " religions { ";

			for (const auto& s : m_rgszReligions)
				ret += s + ' ';

			ret += '}';
		}

		if (m_bCrusading)
			ret += " crusading";

		if (!m_rgszEvents.empty())
		{
			ret += " events { ";

			for (const auto& s : m_rgszEvents)
				ret += s + ' ';

			ret += '}';
		}

		return ret;
	}
};

export struct Pool_t
{
	string m_szName;
	list<string> m_rgszRegions;
	list<Mercenary_t> m_rgMercenaries;

	virtual ~Pool_t(void) noexcept {}

	void Parse(string& sz) noexcept
	{
		if (auto pos = sz.find("pool "); pos != sz.npos)
		{
			pos += strlen_c("pool ");
			sz.erase(0, pos);
			UTIL_Trim(sz);

			m_szName = std::move(sz);
		}
		else if (auto pos = sz.find("regions "); pos != sz.npos)
		{
			pos += strlen_c("regions ");
			sz.erase(0, pos);
			UTIL_Trim(sz);

			UTIL_Split(sz, m_rgszRegions, ' ');
		}
		else if (auto pos = sz.find("unit "); pos != sz.npos)
		{
			m_rgMercenaries.emplace_back(sz);
		}
	}
};

export auto operator<< (OStream auto& lhs, const Pool_t& rhs) noexcept -> decltype(lhs)&
{
	using namespace std;

	lhs << "pool " << rhs.m_szName << endl;

	lhs << "\tregions ";
	for (const auto& szRegion : rhs.m_rgszRegions)
		lhs << szRegion << ' ';
	lhs << endl;

	size_t iBlockAlignedCharCount = 0;
	for (const auto& Mercenary : rhs.m_rgMercenaries)
		iBlockAlignedCharCount = std::max(Mercenary.m_szUnit.length(), iBlockAlignedCharCount);

	iBlockAlignedCharCount += strlen_c("unit ");	// Things in front of unit name.
	if ((iBlockAlignedCharCount % 4) == 0)	// At least one space.
		++iBlockAlignedCharCount;
	while (iBlockAlignedCharCount % 4)
		++iBlockAlignedCharCount;

	for (const auto& Mercenary : rhs.m_rgMercenaries)
		lhs << '\t' << Mercenary.ToString(iBlockAlignedCharCount) << endl;

	return lhs;
}

export struct descr_mercenaries_t
{
	list<Pool_t> m_Pools;

	explicit descr_mercenaries_t(const char* pszFile = "descr_mercenaries.txt") noexcept { Parse(pszFile); }
	virtual ~descr_mercenaries_t(void) noexcept {}

	void Parse(const char* pszFile) noexcept
	{
		std::ifstream f(pszFile);
		if (!f)
			return;

		while (!f.eof())
		{
			string sz;
			std::getline(f, sz);

			if (auto pos = sz.find_first_of(';'); pos != sz.npos)
				sz.erase(pos);

			UTIL_Trim(sz);
			if (sz.empty())
				continue;

			if (sz.starts_with("pool"))
				m_Pools.emplace_back();

			m_Pools.back().Parse(sz);
		}

		f.close();
	}
};

export auto operator<< (OStream auto& lhs, const descr_mercenaries_t& rhs) noexcept -> decltype(lhs)&
{
	using namespace std;

	for (const auto& Pool : rhs.m_Pools)
		lhs << Pool << endl;

	return lhs;
}
