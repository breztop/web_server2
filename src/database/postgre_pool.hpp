#pragma once


#include <breutil/singleton.hpp>
#include <pqxx/pqxx>

class PostgrePool : public Singleton<PostgrePool> {
    friend class Singleton<PostgrePool>;
public:
    ~PostgrePool();

private:
    PostgrePool();
};