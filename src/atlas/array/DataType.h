/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#pragma once

#include <string>


//------------------------------------------------------------------------------------------------------

namespace atlas {
namespace array {

class DataType {
public:
    typedef long kind_t;
    static const kind_t KIND_INT32  = -4;
    static const kind_t KIND_INT64  = -8;
    static const kind_t KIND_REAL32 = 4;
    static const kind_t KIND_REAL64 = 8;
    static const kind_t KIND_UINT64 = -16;

    template <typename DATATYPE>
    static DataType create();

    static DataType int32() { return DataType( KIND_INT32 ); }
    static DataType int64() { return DataType( KIND_INT64 ); }
    static DataType real32() { return DataType( KIND_REAL32 ); }
    static DataType real64() { return DataType( KIND_REAL64 ); }

    template <typename DATATYPE>
    static kind_t kind();
    template <typename DATATYPE>
    static kind_t kind( const DATATYPE& );

    template <typename DATATYPE>
    static std::string str();
    template <typename DATATYPE>
    static std::string str( const DATATYPE );

    static long str_to_kind( const std::string& );
    static std::string kind_to_str( kind_t );
    static bool kind_valid( kind_t );

private:
    static std::string int32_str() { return "int32"; }
    static std::string int64_str() { return "int64"; }
    static std::string real32_str() { return "real32"; }
    static std::string real64_str() { return "real64"; }
    static std::string uint64_str() { return "uint64"; }

    [[noreturn]] static void throw_not_recognised( kind_t );
    [[noreturn]] static void throw_not_recognised( std::string datatype );

public:
    DataType( const std::string& );
    DataType( long );
    DataType( const DataType& );
    std::string str() const { return kind_to_str( kind_ ); }
    kind_t kind() const { return kind_; }
    size_t size() const { return ( kind_ == KIND_UINT64 ) ? 8 : std::abs( kind_ ); }

    friend bool operator==( DataType dt1, DataType dt2 );
    friend bool operator!=( DataType dt1, DataType dt2 );
    friend bool operator==( DataType dt, kind_t kind );
    friend bool operator!=( DataType dt, kind_t kind );
    friend bool operator==( kind_t kind, DataType dt );
    friend bool operator!=( kind_t kind, DataType dt2 );

private:
    kind_t kind_;
};

template <>
inline std::string DataType::str<int>() {
    return int32_str();
}
template <>
inline std::string DataType::str<long>() {
    return int64_str();
}
template <>
inline std::string DataType::str<float>() {
    return real32_str();
}
template <>
inline std::string DataType::str<double>() {
    return real64_str();
}
template <>
inline std::string DataType::str<unsigned long>() {
    return uint64_str();
}
template <>
inline std::string DataType::str( const int& ) {
    return int32_str();
}
template <>
inline std::string DataType::str( const long& ) {
    return int64_str();
}
template <>
inline std::string DataType::str( const unsigned long& ) {
    return uint64_str();
}
template <>
inline std::string DataType::str( const float& ) {
    return real32_str();
}
template <>
inline std::string DataType::str( const double& ) {
    return real64_str();
}
template <>
inline DataType::kind_t DataType::kind<int>() {
    return KIND_INT32;
}
template <>
inline DataType::kind_t DataType::kind<long>() {
    return KIND_INT64;
}
template <>
inline DataType::kind_t DataType::kind<unsigned long>() {
    return KIND_UINT64;
}
template <>
inline DataType::kind_t DataType::kind<float>() {
    return KIND_REAL32;
}
template <>
inline DataType::kind_t DataType::kind<double>() {
    return KIND_REAL64;
}
template <>
inline DataType::kind_t DataType::kind( const int& ) {
    return KIND_INT32;
}
template <>
inline DataType::kind_t DataType::kind( const long& ) {
    return KIND_INT64;
}
template <>
inline DataType::kind_t DataType::kind( const unsigned long& ) {
    return KIND_UINT64;
}
template <>
inline DataType::kind_t DataType::kind( const float& ) {
    return KIND_REAL32;
}
template <>
inline DataType::kind_t DataType::kind( const double& ) {
    return KIND_REAL64;
}

inline DataType::kind_t DataType::str_to_kind( const std::string& datatype ) {
    if ( datatype == "int32" )
        return KIND_INT32;
    else if ( datatype == "int64" )
        return KIND_INT64;
    else if ( datatype == "uint64" )
        return KIND_UINT64;
    else if ( datatype == "real32" )
        return KIND_REAL32;
    else if ( datatype == "real64" )
        return KIND_REAL64;
    else {
        throw_not_recognised( datatype );
    }
}
inline std::string DataType::kind_to_str( kind_t kind ) {
    switch ( kind ) {
        case KIND_INT32:
            return int32_str();
        case KIND_INT64:
            return int64_str();
        case KIND_UINT64:
            return uint64_str();
        case KIND_REAL32:
            return real32_str();
        case KIND_REAL64:
            return real64_str();
        default:
            throw_not_recognised( kind );
    }
}
inline bool DataType::kind_valid( kind_t kind ) {
    switch ( kind ) {
        case KIND_INT32:
        case KIND_INT64:
        case KIND_UINT64:
        case KIND_REAL32:
        case KIND_REAL64:
            return true;
        default:
            return false;
    }
}

inline DataType::DataType( const DataType& other ) : kind_( other.kind_ ) {}

inline DataType::DataType( const std::string& datatype ) : kind_( str_to_kind( datatype ) ) {}

inline DataType::DataType( long kind ) : kind_( kind ) {}

inline bool operator==( DataType dt1, DataType dt2 ) {
    return dt1.kind_ == dt2.kind_;
}

inline bool operator!=( DataType dt1, DataType dt2 ) {
    return dt1.kind_ != dt2.kind_;
}

inline bool operator==( DataType dt, DataType::kind_t kind ) {
    return dt.kind_ == kind;
}

inline bool operator!=( DataType dt, DataType::kind_t kind ) {
    return dt.kind_ != kind;
}

inline bool operator==( DataType::kind_t kind, DataType dt ) {
    return dt.kind_ == kind;
}

inline bool operator!=( DataType::kind_t kind, DataType dt ) {
    return dt.kind_ != kind;
}

template <typename DATATYPE>
inline DataType DataType::create() {
    return DataType( DataType::kind<DATATYPE>() );
}

template <typename DATATYPE>
inline DataType make_datatype() {
    return DataType( DataType::kind<DATATYPE>() );
}

//------------------------------------------------------------------------------------------------------

}  // namespace array
}  // namespace atlas
