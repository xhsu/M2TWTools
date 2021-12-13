module;

#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <tuple>

export module descr_regions;

import UtlColor;
import UtlConcepts;
import UtlString;

using std::ifstream;
using std::list;
using std::string;

struct religion_t
{
	string m_szName;
	short m_iPercentage{ -1 };
};

export auto operator<< (OStream auto& lhs, const religion_t& rhs) noexcept -> decltype(lhs)&
{
	using namespace std;

	lhs << rhs.m_szName << ' ' << rhs.m_iPercentage;

	return lhs;
}

export struct Region_t
{
//private:
	enum RegionMem_e : size_t
	{
		m_szProvincialName = 0U,
		m_szSettlementName,
		m_szCityBuilder,
		m_szRebels,
		m_RegionColor,
		m_rgszHiddenRes,
		m_iVictory,
		m_iAgriculture,
		m_rgReligions,

		_LAST,
	};

	using _MemTys = std::tuple<
		string,				// m_szProvincialName;
		string,				// m_szSettlementName;
		string,				// m_szCityBuilder;
		string,				// m_szRebels;
		Color4b,			// m_RegionColor;
		list<string>,		// m_rgszHiddenRes;
		unsigned short,		// m_iVictory{ 0 };
		unsigned short,		// m_iAgriculture{ 0 };
		list<religion_t>	// m_rgReligions;
	>;

	static_assert(RegionMem_e::_LAST == std::tuple_size_v<_MemTys>, "Something went off.");

	template<size_t i>
	using MemTy = std::tuple_element_t<i, _MemTys>;

	_MemTys m_;

public:
	template<size_t i>
	auto Get(void) noexcept -> MemTy<i>&
	{
		return std::get<i>(m_);
	}

	template<size_t i>
	auto Get(void) const noexcept -> const MemTy<i>&
	{
		return std::get<i>(m_);
	}

	void Parse(IStream auto& file) noexcept
	{
		// Not handliing provincial name.

		const auto fn = [&]<size_t i>(void) -> bool
		{
			string sz;
		LAB_RESTART:;
			if (file.eof())
				return false;

			std::getline(file, sz);

			if (auto iPos = sz.find_first_of(';'); iPos != sz.npos)
				sz.erase(iPos);

			UTIL_Trim(sz);
			if (sz.empty())
				goto LAB_RESTART;

			if constexpr (std::is_same_v<MemTy<i>, string>)
			{
				Get<i>() = std::move(sz);
			}
			else if constexpr (i == m_RegionColor)
			{
				Get<i>() = UTIL_Parse<Color4b>(sz, ' ');
			}
			else if constexpr (i == m_rgszHiddenRes)
			{
				UTIL_Split(sz, Get<i>(), ',');

				for (auto& s : Get<i>())
					UTIL_Trim(s);
			}
			else if constexpr (std::is_same_v<MemTy<i>, unsigned short>)
			{
				Get<i>() = UTIL_StrToNum<unsigned short>(sz);
			}
			else if constexpr (i == m_rgReligions)
			{
				size_t iPos = sz.find_first_of('{') + 1, iEnd = sz.find_first_of('}', iPos);
				char* pBuf = _strdup(sz.substr(iPos, iEnd - iPos).c_str());

				if (Get<i>().empty())
					Get<i>().emplace_back();	// Get an initial value before the loop.

				for (char* p = strtok(pBuf, " "); p != nullptr; p = strtok(nullptr, " "))
				{
					if (!strlen(p))
						continue;

					if (!Get<i>().back().m_szName.empty() && Get<i>().back().m_iPercentage >= 0)
						Get<i>().emplace_back();

					if (Get<i>().back().m_szName.empty())
						Get<i>().back().m_szName = p;
					else if (Get<i>().back().m_iPercentage < 0)
						Get<i>().back().m_iPercentage = UTIL_StrToNum<short>(p);
				}

				free(pBuf);
			}
			else
			{
				static_assert(std::false_type::value, "Corrupted code.");
			}

			return true;
		};

		[&] <size_t... I>(std::index_sequence<I...>)
		{
			(fn.template operator()<I>(), ...);
		}
		(std::make_index_sequence<std::tuple_size_v<_MemTys>>{});
	}

	friend struct descr_regions_t;
};

export auto operator<< (OStream auto& lhs, const Region_t& rhs) noexcept -> decltype(lhs)&
{
	using namespace std;

	lhs << rhs.Get<Region_t::m_szProvincialName>() << endl;
	lhs << '\t' << rhs.Get<Region_t::m_szSettlementName>() << endl;
	lhs << '\t' << rhs.Get<Region_t::m_szCityBuilder>() << endl;
	lhs << '\t' << rhs.Get<Region_t::m_szRebels>() << endl;
	lhs << '\t' << (int)rhs.Get<Region_t::m_RegionColor>()[0] << ' ' << (int)rhs.Get<Region_t::m_RegionColor>()[1] << ' ' << (int)rhs.Get<Region_t::m_RegionColor>()[2] << endl;
	
	lhs << '\t';
	for (auto it = rhs.Get<Region_t::m_rgszHiddenRes>().begin(); it != rhs.Get<Region_t::m_rgszHiddenRes>().end(); ++it)
		lhs << *it << (distance(it, rhs.Get<Region_t::m_rgszHiddenRes>().end()) == 1 ? "" : ", ");
	lhs << endl;

	lhs << '\t' << rhs.Get<Region_t::m_iVictory>() << endl;
	lhs << '\t' << rhs.Get<Region_t::m_iAgriculture>() << endl;

	lhs << "\treligions { ";
	for (auto it = rhs.Get<Region_t::m_rgReligions>().begin(); it != rhs.Get<Region_t::m_rgReligions>().end(); ++it)
		lhs << *it << (distance(it, rhs.Get<Region_t::m_rgReligions>().end()) == 1 ? "" : " ");
	lhs << " }" << endl;

	return lhs;
}

export struct descr_regions_t
{
	list<Region_t> m_Regions;

	explicit descr_regions_t(const char* pszFile = "descr_regions.txt") noexcept { Parse(pszFile); }
	virtual ~descr_regions_t(void) noexcept { m_Regions.clear(); }

	void Parse(const char* pszPath) noexcept
	{
		ifstream f(pszPath);
		if (!f)
			return;

		while (!f.eof())
		{
			m_Regions.emplace_back();
			m_Regions.back().Parse(f);

			if (m_Regions.back().Get<Region_t::m_szProvincialName>().empty())
				m_Regions.pop_back();
		}

		f.close();
	}
};
