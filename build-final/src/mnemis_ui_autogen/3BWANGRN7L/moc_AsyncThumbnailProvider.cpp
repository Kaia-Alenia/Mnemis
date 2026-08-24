/****************************************************************************
** Meta object code from reading C++ file 'AsyncThumbnailProvider.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/ui/providers/AsyncThumbnailProvider.hpp"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AsyncThumbnailProvider.hpp' doesn't include <QObject>."
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
struct qt_meta_stringdata_mnemis__ui__providers__ThumbnailResponse_t {
    uint offsetsAndSizes[8];
    char stringdata0[41];
    char stringdata1[22];
    char stringdata2[1];
    char stringdata3[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_mnemis__ui__providers__ThumbnailResponse_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_mnemis__ui__providers__ThumbnailResponse_t qt_meta_stringdata_mnemis__ui__providers__ThumbnailResponse = {
    {
        QT_MOC_LITERAL(0, 40),  // "mnemis::ui::providers::Thumbn..."
        QT_MOC_LITERAL(41, 21),  // "handleThumbnailResult"
        QT_MOC_LITERAL(63, 0),  // ""
        QT_MOC_LITERAL(64, 5)   // "image"
    },
    "mnemis::ui::providers::ThumbnailResponse",
    "handleThumbnailResult",
    "",
    "image"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_mnemis__ui__providers__ThumbnailResponse[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   20,    2, 0x08,    1 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QImage,    3,

       0        // eod
};

Q_CONSTINIT const QMetaObject mnemis::ui::providers::ThumbnailResponse::staticMetaObject = { {
    QMetaObject::SuperData::link<QQuickImageResponse::staticMetaObject>(),
    qt_meta_stringdata_mnemis__ui__providers__ThumbnailResponse.offsetsAndSizes,
    qt_meta_data_mnemis__ui__providers__ThumbnailResponse,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_mnemis__ui__providers__ThumbnailResponse_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ThumbnailResponse, std::true_type>,
        // method 'handleThumbnailResult'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QImage &, std::false_type>
    >,
    nullptr
} };

void mnemis::ui::providers::ThumbnailResponse::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ThumbnailResponse *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->handleThumbnailResult((*reinterpret_cast< std::add_pointer_t<QImage>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *mnemis::ui::providers::ThumbnailResponse::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *mnemis::ui::providers::ThumbnailResponse::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_mnemis__ui__providers__ThumbnailResponse.stringdata0))
        return static_cast<void*>(this);
    return QQuickImageResponse::qt_metacast(_clname);
}

int mnemis::ui::providers::ThumbnailResponse::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QQuickImageResponse::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
