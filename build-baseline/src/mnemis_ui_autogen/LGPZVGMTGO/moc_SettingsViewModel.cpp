/****************************************************************************
** Meta object code from reading C++ file 'SettingsViewModel.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/ui/controllers/SettingsViewModel.hpp"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SettingsViewModel.hpp' doesn't include <QObject>."
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
struct qt_meta_stringdata_mnemis__ui__controllers__SettingsViewModel_t {
    uint offsetsAndSizes[24];
    char stringdata0[43];
    char stringdata1[20];
    char stringdata2[1];
    char stringdata3[21];
    char stringdata4[17];
    char stringdata5[8];
    char stringdata6[8];
    char stringdata7[5];
    char stringdata8[11];
    char stringdata9[13];
    char stringdata10[13];
    char stringdata11[14];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_mnemis__ui__controllers__SettingsViewModel_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_mnemis__ui__controllers__SettingsViewModel_t qt_meta_stringdata_mnemis__ui__controllers__SettingsViewModel = {
    {
        QT_MOC_LITERAL(0, 42),  // "mnemis::ui::controllers::Sett..."
        QT_MOC_LITERAL(43, 19),  // "libraryRootsChanged"
        QT_MOC_LITERAL(63, 0),  // ""
        QT_MOC_LITERAL(64, 20),  // "includeHiddenChanged"
        QT_MOC_LITERAL(85, 16),  // "setIncludeHidden"
        QT_MOC_LITERAL(102, 7),  // "include"
        QT_MOC_LITERAL(110, 7),  // "addRoot"
        QT_MOC_LITERAL(118, 4),  // "path"
        QT_MOC_LITERAL(123, 10),  // "removeRoot"
        QT_MOC_LITERAL(134, 12),  // "toggleHidden"
        QT_MOC_LITERAL(147, 12),  // "libraryRoots"
        QT_MOC_LITERAL(160, 13)   // "includeHidden"
    },
    "mnemis::ui::controllers::SettingsViewModel",
    "libraryRootsChanged",
    "",
    "includeHiddenChanged",
    "setIncludeHidden",
    "include",
    "addRoot",
    "path",
    "removeRoot",
    "toggleHidden",
    "libraryRoots",
    "includeHidden"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_mnemis__ui__controllers__SettingsViewModel[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       2,   62, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   50,    2, 0x06,    3 /* Public */,
       3,    0,   51,    2, 0x06,    4 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       4,    1,   52,    2, 0x02,    5 /* Public */,
       6,    1,   55,    2, 0x02,    7 /* Public */,
       8,    1,   58,    2, 0x02,    9 /* Public */,
       9,    0,   61,    2, 0x02,   11 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void,

 // properties: name, type, flags
      10, QMetaType::QStringList, 0x00015001, uint(0), 0,
      11, QMetaType::Bool, 0x00015103, uint(1), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject mnemis::ui::controllers::SettingsViewModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_mnemis__ui__controllers__SettingsViewModel.offsetsAndSizes,
    qt_meta_data_mnemis__ui__controllers__SettingsViewModel,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_mnemis__ui__controllers__SettingsViewModel_t,
        // property 'libraryRoots'
        QtPrivate::TypeAndForceComplete<QStringList, std::true_type>,
        // property 'includeHidden'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<SettingsViewModel, std::true_type>,
        // method 'libraryRootsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'includeHiddenChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setIncludeHidden'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'addRoot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'removeRoot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'toggleHidden'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void mnemis::ui::controllers::SettingsViewModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SettingsViewModel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->libraryRootsChanged(); break;
        case 1: _t->includeHiddenChanged(); break;
        case 2: _t->setIncludeHidden((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->addRoot((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->removeRoot((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->toggleHidden(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SettingsViewModel::*)();
            if (_t _q_method = &SettingsViewModel::libraryRootsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SettingsViewModel::*)();
            if (_t _q_method = &SettingsViewModel::includeHiddenChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    }else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<SettingsViewModel *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QStringList*>(_v) = _t->libraryRoots(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->includeHidden(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<SettingsViewModel *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 1: _t->setIncludeHidden(*reinterpret_cast< bool*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *mnemis::ui::controllers::SettingsViewModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *mnemis::ui::controllers::SettingsViewModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_mnemis__ui__controllers__SettingsViewModel.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int mnemis::ui::controllers::SettingsViewModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void mnemis::ui::controllers::SettingsViewModel::libraryRootsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void mnemis::ui::controllers::SettingsViewModel::includeHiddenChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
