#pragma once

#include <functional>


namespace Denon {


template<typename T>
class Property
{
public:
	Property(std::function<void(T)> set, std::function<T(void)> get):
		set(set), get(get)
	{
	}

	operator T() const
	{
		return get();
	}

	Property<T>& operator=(T v)
	{
		set(v);
		return *this;
	};

private:
	std::function<T(void)> get;
	std::function<void(T)> set;
};

template<typename T>
class DeltaProperty: public Property<T>
{
public:
	DeltaProperty(std::function<void(T)> set, std::function<T(void)> get, std::function<void(bool)> delta):
		Property<T>(set, get),
		delta(delta)
	{
	}

	DeltaProperty<T>& operator++()
	{
		delta(true);
		return *this;
	}

	DeltaProperty<T>& operator--()
	{
		delta(false);
		return *this;
	}

private:
	std::function<void(bool)> delta;
};


} // namespace Denon
