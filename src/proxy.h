#ifndef __PROXY_H__
#define __PROXY_H__

namespace ccoscope {

//template<typename WORLD>
/*
class INode {
protected:
    mutable const INode* representative_;
    CodegenContext& ctx_;
    size_t gid_;
}*/

template<class T>
class Proxy {
public:
    typedef T BaseType; // using BaseType = T; ?
   
    Proxy()
        : node_(nullptr)
    {}
    
    Proxy(const T* node)
        : node_(node)
    {
        //static_assert(std::is_base_of<INode, T>::value, "T not derived from INode");
    }

    bool operator == (const Proxy<T>& other) const {
        assert(&(node()->ctx()) == &(other.node()->ctx()));
        //return node()->gid_ == other.node()->gid_;
        return this->deref()->equal(*other.deref());
    }
    bool operator != (const Proxy<T>& other) const { return !(*this == other); }
    const T* representative() const { return node_->representative_->template as<T>(); }
    const T* node() const { assert(node_ != nullptr); return node_; }
    const T* deref() const;
    const T* operator  * () const { return deref(); }
    const T* operator -> () const { return *(*this); }
    
    bool is_empty() const { return node_ == nullptr; }
    //void dump(std::ostream& stream) const;
    //void dump() const { dump(std::cout); std::cout << std::endl; }
    
    // casts

    operator bool() const { return node_; }
    operator const T*() const { return deref(); }

    /// Automatic up-cast in the class hierarchy.
    template<class U> operator Proxy<U>() {
        static_assert(std::is_base_of<U, T>::value, "U is not a base type of T");
        return Proxy<U>((**this)->template as<T>());
    }

    /// Dynamic cast for @p Proxy.
    template<class U> Proxy<typename U::BaseType> isa() const {
        return Proxy<typename U::BaseType>((*this)->template isa<typename U::BaseType>());
    }

    /// Static cast for @p Proxy.
    template<class U> Proxy<typename U::BaseType> as() const {
        return Proxy<typename U::BaseType>((*this)->template as <typename U::BaseType>());
    }

private:
    const T* node_;
};

}

#endif
