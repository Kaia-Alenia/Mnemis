/****************************************************************************
** Meta object code from reading C++ file 'PlaylistController.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/ui/controllers/PlaylistController.hpp"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PlaylistController.hpp' doesn't include <QObject>."
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
struct qt_meta_stringdata_mnemis__ui__controllers__PlaylistController_t {
    uint offsetsAndSizes[28];
    char stringdata0[44];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[10];
    char stringdata4[14];
    char stringdata5[8];
    char stringdata6[14];
    char stringdata7[15];
    char stringdata8[5];
    char stringdata9[15];
    char stringdata10[11];
    char stringdata11[19];
    char stringdata12[8];
    char stringdata13[24];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_mnemis__ui__controllers__PlaylistController_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_mnemis__ui__controllers__PlaylistController_t qt_meta_stringdata_mnemis__ui__controllers__PlaylistController = {
    {
        QT_MOC_LITERAL(0, 43),  // "mnemis::ui::controllers::Play..."
        QT_MOC_LITERAL(44, 15),  // "playlistsLoaded"
        QT_MOC_LITERAL(60, 0),  // ""
        QT_MOC_LITERAL(61, 9),  // "playlists"
        QT_MOC_LITERAL(71, 13),  // "errorOccurred"
        QT_MOC_LITERAL(85, 7),  // "message"
        QT_MOC_LITERAL(93, 13),  // "loadPlaylists"
        QT_MOC_LITERAL(107, 14),  // "createPlaylist"
        QT_MOC_LITERAL(122, 4),  // "name"
        QT_MOC_LITERAL(127, 14),  // "deletePlaylist"
        QT_MOC_LITERAL(142, 10),  // "playlistId"
        QT_MOC_LITERAL(153, 18),  // "addMediaToPlaylist"
        QT_MOC_LITERAL(172, 7),  // "mediaId"
        QT_MOC_LITERAL(180, 23)   // "removeMediaFromPlaylist"
    },
    "mnemis::ui::controllers::PlaylistController",
    "playlistsLoaded",
    "",
    "playlists",
    "errorOccurred",
    "message",
    "loadPlaylists",
    "createPlaylist",
    "name",
    "deletePlaylist",
    "playlistId",
    "addMediaToPlaylist",
    "mediaId",
    "removeMediaFromPlaylist"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_mnemis__ui__controllers__PlaylistController[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   56,    2, 0x06,    1 /* Public */,
       4,    1,   59,    2, 0x06,    3 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       6,    0,   62,    2, 0x02,    5 /* Public */,
       7,    1,   63,    2, 0x02,    6 /* Public */,
       9,    1,   66,    2, 0x02,    8 /* Public */,
      11,    2,   69,    2, 0x02,   10 /* Public */,
      13,    2,   74,    2, 0x02,   13 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QVariantList,    3,
    QMetaType::Void, QMetaType::QString,    5,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   10,   12,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   10,   12,

       0        // eod
};

Q_CONSTINIT const QMetaObject mnemis::ui::controllers::PlaylistController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_mnemis__ui__controllers__PlaylistController.offsetsAndSizes,
    qt_meta_data_mnemis__ui__controllers__PlaylistController,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_mnemis__ui__controllers__PlaylistController_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PlaylistController, std::true_type>,
        // method 'playlistsLoaded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantList &, std::false_type>,
        // method 'errorOccurred'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'loadPlaylists'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'createPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'deletePlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'addMediaToPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'removeMediaFromPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void mnemis::ui::controllers::PlaylistController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<PlaylistController *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->playlistsLoaded((*reinterpret_cast< std::add_pointer_t<QVariantList>>(_a[1]))); break;
        case 1: _t->errorOccurred((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->loadPlaylists(); break;
        case 3: _t->createPlaylist((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->deletePlaylist((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->addMediaToPlaylist((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 6: _t->removeMediaFromPlaylist((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (PlaylistController::*)(const QVariantList & );
            if (_t _q_method = &PlaylistController::playlistsLoaded; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (PlaylistController::*)(const QString & );
            if (_t _q_method = &PlaylistController::errorOccurred; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject *mnemis::ui::controllers::PlaylistController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *mnemis::ui::controllers::PlaylistController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_mnemis__ui__controllers__PlaylistController.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int mnemis::ui::controllers::PlaylistController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void mnemis::ui::controllers::PlaylistController::playlistsLoaded(const QVariantList & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void mnemis::ui::controllers::PlaylistController::errorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
