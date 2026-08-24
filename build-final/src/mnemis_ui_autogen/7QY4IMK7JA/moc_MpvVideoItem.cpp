/****************************************************************************
** Meta object code from reading C++ file 'MpvVideoItem.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/ui/components/MpvVideoItem.hpp"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MpvVideoItem.hpp' doesn't include <QObject>."
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
struct qt_meta_stringdata_mnemis__ui__components__MpvVideoItem_t {
    uint offsetsAndSizes[14];
    char stringdata0[37];
    char stringdata1[18];
    char stringdata2[1];
    char stringdata3[16];
    char stringdata4[18];
    char stringdata5[11];
    char stringdata6[45];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_mnemis__ui__components__MpvVideoItem_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_mnemis__ui__components__MpvVideoItem_t qt_meta_stringdata_mnemis__ui__components__MpvVideoItem = {
    {
        QT_MOC_LITERAL(0, 36),  // "mnemis::ui::components::MpvVi..."
        QT_MOC_LITERAL(37, 17),  // "controllerChanged"
        QT_MOC_LITERAL(55, 0),  // ""
        QT_MOC_LITERAL(56, 15),  // "renderRequested"
        QT_MOC_LITERAL(72, 17),  // "onRenderRequested"
        QT_MOC_LITERAL(90, 10),  // "controller"
        QT_MOC_LITERAL(101, 44)   // "mnemis::ui::controllers::Play..."
    },
    "mnemis::ui::components::MpvVideoItem",
    "controllerChanged",
    "",
    "renderRequested",
    "onRenderRequested",
    "controller",
    "mnemis::ui::controllers::PlaybackController*"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_mnemis__ui__components__MpvVideoItem[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       1,   35, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   32,    2, 0x06,    2 /* Public */,
       3,    0,   33,    2, 0x06,    3 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       4,    0,   34,    2, 0x0a,    4 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,

 // properties: name, type, flags
       5, 0x80000000 | 6, 0x0001510b, uint(0), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject mnemis::ui::components::MpvVideoItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QQuickFramebufferObject::staticMetaObject>(),
    qt_meta_stringdata_mnemis__ui__components__MpvVideoItem.offsetsAndSizes,
    qt_meta_data_mnemis__ui__components__MpvVideoItem,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_mnemis__ui__components__MpvVideoItem_t,
        // property 'controller'
        QtPrivate::TypeAndForceComplete<mnemis::ui::controllers::PlaybackController*, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MpvVideoItem, std::true_type>,
        // method 'controllerChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'renderRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRenderRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void mnemis::ui::components::MpvVideoItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MpvVideoItem *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->controllerChanged(); break;
        case 1: _t->renderRequested(); break;
        case 2: _t->onRenderRequested(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MpvVideoItem::*)();
            if (_t _q_method = &MpvVideoItem::controllerChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MpvVideoItem::*)();
            if (_t _q_method = &MpvVideoItem::renderRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    } else if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< mnemis::ui::controllers::PlaybackController* >(); break;
        }
    }
else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<MpvVideoItem *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< mnemis::ui::controllers::PlaybackController**>(_v) = _t->controller(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<MpvVideoItem *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setController(*reinterpret_cast< mnemis::ui::controllers::PlaybackController**>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *mnemis::ui::components::MpvVideoItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *mnemis::ui::components::MpvVideoItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_mnemis__ui__components__MpvVideoItem.stringdata0))
        return static_cast<void*>(this);
    return QQuickFramebufferObject::qt_metacast(_clname);
}

int mnemis::ui::components::MpvVideoItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QQuickFramebufferObject::qt_metacall(_c, _id, _a);
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
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void mnemis::ui::components::MpvVideoItem::controllerChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void mnemis::ui::components::MpvVideoItem::renderRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
