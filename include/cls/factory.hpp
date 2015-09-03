﻿/////////////////////////////////////////////////////////////////////////////////
// The MIT License(MIT)
//
// Copyright (c) 2014 Tiangang Song
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
/////////////////////////////////////////////////////////////////////////////////

#ifndef CLS_FACTORY_HPP
#define CLS_FACTORY_HPP

#include <map>
#include <memory>
#include <functional>
#include "cls_defs.h"

_CLS_BEGIN
namespace detail {
template<typename T, template<typename> class Ptr>
struct FactoryCreator {
    template<typename... Args>
    Ptr<T> operator()(Args&& ... args)
    {
        return Ptr<T>(new T(forward<Args>(args)...));
    };

    template<typename... Args>
    Ptr<T> create(Args&& ... args)
    {
        return (*this)(forward<Args>(args)...);
    };
};


template<typename BaseType, typename IDType, template<typename> class Ptr, typename... CtorArgs>
class ObjFactory {
public:
    using BasePtr = Ptr<BaseType>;
    using Creator = function<BasePtr(CtorArgs...)>;

    static bool addType(const IDType& ID, const Creator& creator)
    {
        auto& creators = instance().creators_map;
        if (creators.find(ID) == creators.end()) {
            creators.insert({ID, creator});
            return true;
        }

        return false;
    }

    template<typename T>
    static bool addType(const IDType& ID)
    {
        return addType(ID, FactoryCreator<T, Ptr>());
    }

    static bool removeType(const IDType& ID)
    {
        auto& creators = instance().creators_map;
        if (creators.find(ID) != creators.end()) {
            creators.erase(ID);
            return true;
        }

        return false;
    }

    template<typename... Args>
    static BasePtr create(const IDType& ID, Args&& ... args)
    {
        auto& creators = instance().creators_map;
        auto iter = creators.find(ID);
        if (iter != creators.end()) {
            return (iter->second)(forward<Args>(args)...);
        }

        return nullptr;
    }

private:
    ObjFactory() = default;

    ObjFactory(const ObjFactory&) = delete;

    ObjFactory& operator=(const ObjFactory&) = delete;

    static ObjFactory& instance()
    {
        static ObjFactory obj_factory;
        return obj_factory;
    }

    map<IDType, Creator> creators_map;
};

template<typename T>
using unique_ptr = std::unique_ptr<T>;
}

template<typename Base, typename IDType = string, template<typename> class Ptr = detail::unique_ptr>
struct Factory {
    template<typename Derived, typename... CtorArgs>
    static bool addType(const IDType& id)
    {
        return detail::ObjFactory<Base, IDType, Ptr, CtorArgs...>::addType(
                id, detail::FactoryCreator<Derived, Ptr>());
    };

    template<typename... CtorArgs>
    static auto create(const IDType& id, CtorArgs&& ... args) -> Ptr<Base>
    {
        return detail::ObjFactory<Base, IDType, Ptr, CtorArgs...>::create(
                id, forward<CtorArgs>(args)...);
    }

    template<typename... CtorArgs>
    static bool removeType(const IDType& ID)
    {
        return detail::ObjFactory<Base, IDType, Ptr, CtorArgs...>::removeType(ID);
    }
};

_CLS_END


// Use these macro in *.cpp file
#define REGISTER_TO_FACTORY(BaseType, DerivedType) \
namespace { \
    const bool ADD_##DerivedType = cls::ObjFactory<BaseType>::addType<DerivedType>(#DerivedType); \
}

// Have "typedef BaseType base" in derived class
#define REGISTER_TYPE_TO_FACTORY(DerivedType) \
namespace { \
    const bool ADD_##DerivedType = cls::ObjFactory<DerivedType::base>::addType<DerivedType>(#DerivedType); \
}

// Custom creator
#define REGISTER_TO_FACTORY_WITH_CTOR(BaseType, DerivedType, Creator) \
namespace { \
    const bool ADD_##DerivedType = cls::ObjFactory<BaseType, string, Creator>::addType<DerivedType>(#DerivedType); \
}

#endif // CLS_FACTORY_HPP