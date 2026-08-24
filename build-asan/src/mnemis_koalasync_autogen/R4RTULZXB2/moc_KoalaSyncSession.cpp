/****************************************************************************
** Meta object code from reading C++ file 'KoalaSyncSession.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/plugins/koalasync_plugin/KoalaSyncSession.hpp"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'KoalaSyncSession.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_mnemis__plugins__koalasync__KoalaSyncSession_t {
    uint offsetsAndSizes[18];
    char stringdata0[45];
    char stringdata1[10];
    char stringdata2[1];
    char stringdata3[13];
    char stringdata4[7];
    char stringdata5[11];
    char stringdata6[12];
    char stringdata7[11];
    char stringdata8[9];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_mnemis__plugins__koalasync__KoalaSyncSession_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_mnemis__plugins__koalasync__KoalaSyncSession_t qt_meta_stringdata_mnemis__plugins__koalasync__KoalaSyncSession = {
    {
        QT_MOC_LITERAL(0, 44),  // "mnemis::plugins::koalasync::K..."
        QT_MOC_LITERAL(45, 9),  // "connected"
        QT_MOC_LITERAL(55, 0),  // ""
        QT_MOC_LITERAL(56, 12),  // "disconnected"
        QT_MOC_LITERAL(69, 6),  // "reason"
        QT_MOC_LITERAL(76, 10),  // "remotePlay"
        QT_MOC_LITERAL(87, 11),  // "remotePause"
        QT_MOC_LITERAL(99, 10),  // "remoteSeek"
        QT_MOC_LITERAL(110, 8)   // "position"
    },
    "mnemis::plugins::koalasync::KoalaSyncSession",
    "connected",
    "",
    "disconnected",
    "reason",
    "remotePlay",
    "remotePause",
    "remoteSeek",
    "position"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_mnemis__plugins__koalasync__KoalaSyncSession[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   44,    2, 0x06,    1 /* Public */,
       3,    1,   45,    2, 0x06,    2 /* Public */,
       5,    0,   48,    2, 0x06,    4 /* Public */,
       6,    0,   49,    2, 0x06,    5 /* Public */,
       7,    1,   50,    2, 0x06,    6 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double,    8,

       0        // eod
};

Q_CONSTINIT const QMetaObject mnemis::plugins::koalasync::KoalaSyncSession::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_mnemis__plugins__koalasync__KoalaSyncSession.offsetsAndSizes,
    qt_meta_data_mnemis__plugins__koalasync__KoalaSyncSession,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_mnemis__plugins__koalasync__KoalaSyncSession_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<KoalaSyncSession, std::true_type>,
        // method 'connected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'disconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'remotePlay'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'remotePause'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'remoteSeek'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>
    >,
    nullptr
} };

void mnemis::plugins::koalasync::KoalaSyncSession::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<KoalaSyncSession *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->connected(); break;
        case 1: _t->disconnected((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->remotePlay(); break;
        case 3: _t->remotePause(); break;
        case 4: _t->remoteSeek((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (KoalaSyncSession::*)();
            if (_t _q_method = &KoalaSyncSession::connected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (KoalaSyncSession::*)(const QString & );
            if (_t _q_method = &KoalaSyncSession::disconnected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (KoalaSyncSession::*)();
            if (_t _q_method = &KoalaSyncSession::remotePlay; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (KoalaSyncSession::*)();
            if (_t _q_method = &KoalaSyncSession::remotePause; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (KoalaSyncSession::*)(double );
            if (_t _q_method = &KoalaSyncSession::remoteSeek; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject *mnemis::plugins::koalasync::KoalaSyncSession::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *mnemis::plugins::koalasync::KoalaSyncSession::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_mnemis__plugins__koalasync__KoalaSyncSession.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int mnemis::plugins::koalasync::KoalaSyncSession::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void mnemis::plugins::koalasync::KoalaSyncSession::connected()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void mnemis::plugins::koalasync::KoalaSyncSession::disconnected(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void mnemis::plugins::koalasync::KoalaSyncSession::remotePlay()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void mnemis::plugins::koalasync::KoalaSyncSession::remotePause()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void mnemis::plugins::koalasync::KoalaSyncSession::remoteSeek(double _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
