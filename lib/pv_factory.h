// -*- c++ -*-
//
// pv_factory.h
//
// kasemir@lanl.gov

#ifndef __PV_FACTORY_H__
#define __PV_FACTORY_H__

#include<time.h>
#include"hashtable.h"
#include"dl_list.h"

// PV_Factory: Factory for ProcessVariables.
// When e.g. a widget asks for PV by name for the first time,
// a new PV will be created. Next time,
// an existing one will be returned.
//
// The actual PV_Factory that creates the ProcessVariable
// might be EPICS_PV_Factory or CALC_PV_Factory or ...
//
// See epics_pv_factory, calc_pv_factory, ...
// for example implementations.
class PV_Factory
{
public:
    PV_Factory();
    virtual ~PV_Factory();

    enum { MAX_PV_NAME=100 };
    
    // Result is referenced once, call release() when no longer needed.
    virtual class ProcessVariable *create(const char *PV_name);
    virtual class ProcessVariable *createWithInitialCallbacks (
      const char *PV_name );

};

// If you have to use fixed-length strings, use
// char name[PV_Factory::MAX_PV_NAME];
//
// The enum hack is from Meyer's "Effective C++" book,
// it's more portable than "static const int".

// All ProcessVariables are to be created
// by calls to this central PV_Factory:
// (Which is for now created in pv_factory.cc)
extern PV_Factory *the_PV_Factory;

// ProcessVariable:
// Created via PV_Factory, reference-counted.
//
// Will handle
// * connection to ChannelAccess (or other control system),
// * initial request for value information (DBR_CTRL_<native type> for EPICS),
// * subscription to native-typed(!) value of the channel.
// User can query the current state, value, control/display info
// and add/remove callbacks if interested.
//
// Two types of callback are supported:
// 1) "value"
//    Indicates a change in value, status/severity or time stamp.
//    This is expected to happen quite often,
//    widgets will require some redraw to reflect the new value.
// 2) "conn_state"
//    Indicates a change in "connection" or "state".
//    Check is_valid():
//    false - PV got disconnected.
//            Widget might display PV name to show what got disconnected
//    true  - PV has new configuration
//            (units, display/warning/alarm limits, precision, ...)
//            This event is issued with the initial connection
//            as well as with each re-connection.
//            Some systems might allow online configuration changes.
//            In any case the widget might need a full redraw
//            to reflect new axes limits etc.

// status and severity for now match the EPICS model
// as defined in base/include/alarm.h:
#ifndef INVALID_ALARM
#define NO_ALARM		0x0
#define	MINOR_ALARM		0x1
#define	MAJOR_ALARM		0x2
#define INVALID_ALARM	0x3
#define	UDF_ALARM		17
#endif

class ProcessVariable
{
public:
    const char *get_name() const;
    
    // When no longer used, release, don't delete:
    void reference();
    void release();

    // Implies 1) connected 2) have received valid control info
    virtual bool is_valid() const = 0;

    // Called on change in value, see above for details
    typedef void (*Callback)(ProcessVariable *pv, void *userarg);
    void add_value_callback(Callback func, void *userarg);
    void remove_value_callback(Callback func, void *userarg);

    // Called on change in "is_valid" status, see above
    void add_conn_state_callback(Callback func, void *userarg);
    void remove_conn_state_callback(Callback func, void *userarg);

    // Type information for this ProcessVariable
    // For EPICS, we could just use DBF_INT, DBF_DOUBLE etc.
    // but this is meant to descibe PV types of other systems as well.
    typedef struct
    {
        // Character of this PV
        enum { real, integer, enumerated, text, special } type;
        // real ... enumerated: bit size
        // text               : max length, 0 for variable length
        // special            : no clue
        size_t size;
        // Human-readable description. Currently supported:
        // "integer:32"
        // "real:64"  for double
        // "enumerated:16"
        // "text:0"
        const char *description;
    } Type;

    virtual const Type &get_type() const = 0;
   
    // -- Don't call ANY of the following when is_valid() returns false!!

    // Whenever possible, ProcessVariable internally holds the "native" value
    // of the control system. So no matter if the raw value was an int
    // or double or enumrated, you can ask for int, double or a string.
    // 
    // It the value is really enumerated, you can ask for the enumeration
    // strings. Otherwise the enumeration strings are undefined.
    // For arrays, you have to respect the native type.
    // Example: call get_int_array() only if get_type() reports integer.
    
    // -- These numeric values may be UNDEFINED for
    //    get_type().type == text or special
    virtual int         get_int() const;
    virtual double      get_double() const = 0;
    // writes strbuf, formatted according to precision etc.
    // returns actual strlen
    // Should always work for all types!
    virtual size_t      get_string(char *strbuf, size_t buflen) const;

    // Support one-dim. arrays.
    // For scalars, get_dimension() returns 1
    // (In the future, something like get_dimension(int i) might be support
    //  multi-dim. arrays.)
    virtual size_t      get_dimension() const = 0;

    // Use this when get_type().type == integer
    virtual const int    *get_int_array() const = 0;
    // Use this when get_type().type == real
    virtual const double *get_double_array() const = 0;
    
    // Get number and (if > 0) strings for enumerated value
    virtual size_t      get_enum_count() const;
    virtual const char *get_enum(size_t i) const;
    // Time: support for seconds in ANSI time_t format (since 1970 UTC)
    // as well as nano second extension
	virtual time_t get_time_t() const = 0;
	virtual unsigned long get_nano() const = 0;
    // Status info, various limits
    virtual short       get_status() const = 0;
    // Severity is NO_ALARM, MINOR_ALARM, MAJOR_ALARM or INVALID_ALARM
    virtual short       get_severity() const = 0;
    virtual short       get_precision() const = 0;
    virtual const char *get_units() const;
    virtual double      get_upper_disp_limit() const = 0;
    virtual double      get_lower_disp_limit() const = 0;
    virtual double      get_upper_alarm_limit() const = 0;
    virtual double      get_upper_warning_limit() const = 0;
    virtual double      get_lower_warning_limit() const = 0;
    virtual double      get_lower_alarm_limit() const = 0;
    virtual double      get_upper_ctrl_limit() const = 0;
    virtual double      get_lower_ctrl_limit() const = 0;

    // Plan: support getting attributes like status, precision, ...
    //       via an attribute-itentifier
    // virtual double      get_attribute(Attribute attribute);
    
    // Output/write methods
    // Some systems might implement access rights,
    // e.g. based on the current user ID.
    // A control widget might choose to indicate
    // if there is no write access to this PV
    virtual bool have_write_access() const;
    virtual bool put(double value) = 0;
    virtual bool put(int value);
    virtual bool put(const char *value) = 0;

    void clearDoInitialCallback();
    void setDoInitialCallback();
    
protected:
    // hidden, use PV_Factory::create()/ProcessVariable::release()
    ProcessVariable(const char *_name);
    ProcessVariable(const ProcessVariable &rhs); // not impl.
    ProcessVariable &operator = (const ProcessVariable &rhs); // not impl.
    virtual ~ProcessVariable();

    class CallbackInfo
    {
    public:
        CallbackInfo(Callback _func, void *_userarg)
        {  func = _func; userarg = _userarg; }
        void call(ProcessVariable *pv)
        {
            if (func)
                func(pv, userarg);
        }
        Callback func;
        void    *userarg;
        DLNode   node; // for Hashtable
    };
    friend size_t hash(const CallbackInfo *item, size_t N);
    friend bool equals(const CallbackInfo *lhs, const CallbackInfo *rhs);
    enum { HashTableSize=43 };
    typedef Hashtable<CallbackInfo,offsetof(CallbackInfo,node),HashTableSize>
            CallbackInfoHash;
    CallbackInfoHash value_callbacks;
    CallbackInfoHash conn_state_callbacks;
    void do_value_callbacks();
    void do_conn_state_callbacks();

    bool doInitialCallback;
        
private:
    char *name;            // PV name
    int refcount;          // reference count, deleted on <=0
};

// ------------- Inlines -----------------------------------------------
inline const char *ProcessVariable::get_name() const
{   return name; }

inline void ProcessVariable::reference()
{   ++refcount; }

inline void ProcessVariable::release()
{   if (--refcount <= 0) delete this; }

#endif
