// Purpose: simple callback implementation
// Licence: Public Domain
// Author: Marcin Wojdyr; based on http://www.newty.de/jakubik/callback.pdf
// $Id$
//

/// See e.g. http://www.tutok.sk/fastgl/callback.html
/// and http://www.newty.de/jakubik/callback.html for discussion
/// of various callback mechanisms in C++

#ifndef FITYK__WX_CALLBACK__H__
#define FITYK__WX_CALLBACK__H__

// V1Callback -- callback with one parameter and return type void

template <typename T>
class V1CallbackBodyB
{
public:
  virtual ~V1CallbackBodyB() {}
  virtual void call(T) const = 0;
  virtual V1CallbackBodyB<T>* clone() const = 0;
};

template <typename T, typename Class, typename Member>
class V1CallbackBody : public V1CallbackBodyB<T>
{
public:
  V1CallbackBody(Class* instance_, Member member_)
      : instance(instance_), member(member_) {}

  void call(T t) const { (instance->*member)(t); }

  V1CallbackBody<T,Class,Member>* clone() const
      { return new V1CallbackBody<T,Class,Member>(*this); }

private:
  Class *const instance;
  Member member;
};


template <typename T>
class V1Callback
{
public:
  V1Callback(V1CallbackBodyB<T>* body_) : body(body_) {}
  ~V1Callback() { delete body; }
  V1Callback(const V1Callback<T>& callback) : body(callback.body->clone()) {}
  void operator()(T p1) {body->call(p1);}
private:
  V1CallbackBodyB<T>* body;
  V1Callback<T>& operator=(const V1Callback<T>& callback); //disable
};

#if 0
template <typename T, typename Class, typename Member>
V1Callback<T> make_callback(V1Callback<T>*, Class* instance, Member member)
{
  return V1Callback<T> (new V1CallbackBody<T,Class,Member>(instance, member));
}
// invoked like this:
//make_callback((V1Callback<wxString const&>*)0, this, &IOPane::OnInputLine));
#endif

template <typename T>
class make_callback
{
public:
    template <typename Class, typename Member>
    V1Callback<T> V1(Class* instance, Member member)
    {
        return V1Callback<T> (new V1CallbackBody<T,Class,Member>
                                                    (instance, member));
    }
};

#endif
