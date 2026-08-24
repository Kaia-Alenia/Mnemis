/****************************************************************************
** Meta object code from reading C++ file 'AnimatedFrameItem.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/ui/components/AnimatedFrameItem.hpp"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AnimatedFrameItem.hpp' doesn't include <QObject>."
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
struct qt_meta_stringdata_mnemis__ui__components__AnimatedFrameItem_t {
    uint offsetsAndSizes[18];
    char stringdata0[42];
    char stringdata1[18];
    char stringdata2[1];
    char stringdata3[14];
    char stringdata4[13];
    char stringdata5[6];
    char stringdata6[11];
    char stringdata7[50];
    char stringdata8[7];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_mnemis__ui__components__AnimatedFrameItem_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_mnemis__ui__components__AnimatedFrameItem_t qt_meta_stringdata_mnemis__ui__components__AnimatedFrameItem = {
    {
        QT_MOC_LITERAL(0, 41),  // "mnemis::ui::components::Anima..."
        QT_MOC_LITERAL(42, 17),  // "controllerChanged"
        QT_MOC_LITERAL(60, 0),  // ""
        QT_MOC_LITERAL(61, 13),  // "smoothChanged"
        QT_MOC_LITERAL(75, 12),  // "onFrameReady"
        QT_MOC_LITERAL(88, 5),  // "frame"
        QT_MOC_LITERAL(94, 10),  // "controller"
        QT_MOC_LITERAL(105, 49),  // "mnemis::ui::controllers::Anim..."
        QT_MOC_LITERAL(155, 6)   // "smooth"
    },
    "mnemis::ui::components::AnimatedFrameItem",
    "controllerChanged",
    "",
    "smoothChanged",
    "onFrameReady",
    "frame",
    "controller",
    "mnemis::ui::controllers::AnimatedMediaController*",
    "smooth"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_mnemis__ui__components__AnimatedFrameItem[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       2,   37, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   32,    2, 0x06,    3 /* Public */,
       3,    0,   33,    2, 0x06,    4 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       4,    1,   34,    2, 0x08,    5 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QImage,    5,

 // properties: name, type, flags
       6, 0x80000000 | 7, 0x0001510b, uint(0), 0,
       8, QMetaType::Bool, 0x00015103, uint(1), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject mnemis::ui::components::AnimatedFrameItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QQuickPaintedItem::staticMetaObject>(),
    qt_meta_stringdata_mnemis__ui__components__AnimatedFrameItem.offsetsAndSizes,
    qt_meta_data_mnemis__ui__components__AnimatedFrameItem,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_mnemis__ui__components__AnimatedFrameItem_t,
        // property 'controller'
        QtPrivate::TypeAndForceComplete<mnemis::ui::controllers::AnimatedMediaController*, std::true_type>,
        // property 'smooth'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AnimatedFrameItem, std::true_type>,
        // method 'controllerChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'smoothChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onFrameReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QImage &, std::false_type>
    >,
    nullptr
} };

void mnemis::ui::components::AnimatedFrameItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AnimatedFrameItem *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->controllerChanged(); break;
        case 1: _t->smoothChanged(); break;
        case 2: _t->onFrameReady((*reinterpret_cast< std::add_pointer_t<QImage>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AnimatedFrameItem::*)();
            if (_t _q_method = &AnimatedFrameItem::controllerChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AnimatedFrameItem::*)();
            if (_t _q_method = &AnimatedFrameItem::smoothChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    } else if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< mnemis::ui::controllers::AnimatedMediaController* >(); break;
        }
    }
else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<AnimatedFrameItem *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< mnemis::ui::controllers::AnimatedMediaController**>(_v) = _t->controller(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->smooth(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<AnimatedFrameItem *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setController(*reinterpret_cast< mnemis::ui::controllers::AnimatedMediaController**>(_v)); break;
        case 1: _t->setSmooth(*reinterpret_cast< bool*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *mnemis::ui::components::AnimatedFrameItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *mnemis::ui::components::AnimatedFrameItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_mnemis__ui__components__AnimatedFrameItem.stringdata0))
        return static_cast<void*>(this);
    return QQuickPaintedItem::qt_metacast(_clname);
}

int mnemis::ui::components::AnimatedFrameItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QQuickPaintedItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void mnemis::ui::components::AnimatedFrameItem::controllerChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void mnemis::ui::components::AnimatedFrameItem::smoothChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
