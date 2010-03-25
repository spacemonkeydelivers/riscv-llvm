// RUN: %clang_cc1 -fsyntax-only -verify %s

// PR6619
template<bool C> struct if_c { };
template<typename T1> struct if_ {
  typedef if_c< static_cast<bool>(T1::value)> almost_type_; // expected-note 7{{in instantiation}}
};
template <class Model, void (Model::*)()> struct wrap_constraints { };
template <class Model> 
inline char has_constraints_(Model* ,  // expected-note 4{{while substituting}} \
                             // expected-note 3{{candidate template ignored}}
                               wrap_constraints<Model,&Model::constraints>* = 0); // expected-note 4{{in instantiation}}

template <class Model> struct not_satisfied {
  static const bool value = sizeof( has_constraints_((Model*)0)  == 1); // expected-error 3{{no matching function}}
};
template <class ModelFn> struct requirement_;
template <void(*)()> struct instantiate {
};
template <class Model> struct requirement_<void(*)(Model)>                           : if_<       not_satisfied<Model>         >::type { // expected-error 3{{no type named}} \
  // expected-note 7{{in instantiation}}
};
template <class Model> struct usage_requirements {
};
template < typename TT > struct InputIterator                            {
    typedef  instantiate< & requirement_<void(*)(usage_requirements<InputIterator> x)>::failed> boost_concept_check1; // expected-note 2{{in instantiation}}
};
template < typename TT > struct ForwardIterator                              : InputIterator<TT>                              { // expected-note 2{{in instantiation}}
  typedef instantiate< & requirement_<void(*)(usage_requirements<ForwardIterator> x)>::failed> boost_concept_check2; // expected-note 2 {{in instantiation}}

};
typedef instantiate< &requirement_<void(*)(ForwardIterator<char*> x)>::failed> boost_concept_checkX; // expected-error{{no member named}} \
// expected-note 6{{in instantiation}}

template<typename T> struct X0 { };
template<typename R, typename A1> struct X0<R(A1 param)> { };

template<typename T, typename A1, typename A2>
void instF0(X0<T(A1)> x0a, X0<T(A2)> x0b) {
  X0<T(A1)> x0c;
  X0<T(A2)> x0d;
}

template void instF0<int, int, float>(X0<int(int)>, X0<int(float)>);

template<typename R, typename A1, R (*ptr)(A1)> struct FuncPtr { };
template<typename A1, int (*ptr)(A1)> struct FuncPtr<int, A1, ptr> { };

template<typename R, typename A1> R unary_func(A1);

template<typename R, typename A1, typename A2>
void use_func_ptr() {
  FuncPtr<R, A1, &unary_func<R, A1> > fp1;
  FuncPtr<R, A2, &unary_func<R, A2> > fp2;
};

template void use_func_ptr<int, float, double>();
