/****************************************************************************
** Meta object code from reading C++ file 'WebSocketTransport.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/plugins/koalasync_plugin/transport/WebSocketTransport.hpp"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WebSocketTransport.hpp' doesn't include <QObject>."
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
struct qt_meta_stringdata_mnemis__plugins__koalasync__transport__WebSocketTransport_t {
    uint offsetsAndSizes[18];
    char stringdata0[58];
    char stringdata1[12];
    char stringdata2[1];
    char stringdata3[15];
    char stringdata4[22];
    char stringdata5[8];
    char stringdata6[16];
    char stringdata7[29];
    char stringdata8[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_mnemis__plugins__koalasync__transport__WebSocketTransport_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_mnemis__plugins__koalasync__transport__WebSocketTransport_t qt_meta_stringdata_mnemis__plugins__koalasync__transport__WebSocketTransport = {
    {
        QT_MOC_LITERAL(0, 57),  // "mnemis::plugins::koalasync::t..."
        QT_MOC_LITERAL(58, 11),  // "onConnected"
        QT_MOC_LITERAL(70, 0),  // ""
        QT_MOC_LITERAL(71, 14),  // "onDisconnected"
        QT_MOC_LITERAL(86, 21),  // "onTextMessageReceived"
        QT_MOC_LITERAL(108, 7),  // "message"
        QT_MOC_LITERAL(116, 15),  // "onErrorOccurred"
        QT_MOC_LITERAL(132, 28),  // "QAbstractSocket::SocketError"
        QT_MOC_LITERAL(161, 5)   // "error"
    },
    "mnemis::plugins::koalasync::transport::WebSocketTransport",
    "onConnected",
    "",
    "onDisconnected",
    "onTextMessageReceived",
    "message",
    "onErrorOccurred",
    "QAbstractSocket::SocketError",
    "error"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_mnemis__plugins__koalasync__transport__WebSocketTransport[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   38,    2, 0x08,    1 /* Private */,
       3,    0,   39,    2, 0x08,    2 /* Private */,
       4,    1,   40,    2, 0x08,    3 /* Private */,
       6,    1,   43,    2, 0x08,    5 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, 0x80000000 | 7,    8,

       0        // eod
};

Q_CONSTINIT const QMetaObject mnemis::plugins::koalasync::transport::WebSocketTransport::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_mnemis__plugins__koalasync__transport__WebSocketTransport.offsetsAndSizes,
    qt_meta_data_mnemis__plugins__koalasync__transport__WebSocketTransport,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_mnemis__plugins__koalasync__transport__WebSocketTransport_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<WebSocketTransport, std::true_type>,
        // method 'onConnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onTextMessageReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onErrorOccurred'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QAbstractSocket::SocketError, std::false_type>
    >,
    nullptr
} };

void mnemis::plugins::koalasync::transport::WebSocketTransport::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<WebSocketTransport *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onConnected(); break;
        case 1: _t->onDisconnected(); break;
        case 2: _t->onTextMessageReceived((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->onErrorOccurred((*reinterpret_cast< std::add_pointer_t<QAbstractSocket::SocketError>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAbstractSocket::SocketError >(); break;
            }
            break;
        }
    }
}

const QMetaObject *mnemis::plugins::koalasync::transport::WebSocketTransport::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *mnemis::plugins::koalasync::transport::WebSocketTransport::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_mnemis__plugins__koalasync__transport__WebSocketTransport.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "ITransport"))
        return static_cast< ITransport*>(this);
    return QObject::qt_metacast(_clname);
}

int mnemis::plugins::koalasync::transport::WebSocketTransport::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
