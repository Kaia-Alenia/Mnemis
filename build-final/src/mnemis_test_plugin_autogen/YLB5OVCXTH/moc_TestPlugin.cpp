/****************************************************************************
** Meta object code from reading C++ file 'TestPlugin.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/plugins/test_plugin/TestPlugin.hpp"
#include <QtCore/qmetatype.h>
#include <QtCore/qplugin.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TestPlugin.hpp' doesn't include <QObject>."
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
struct qt_meta_stringdata_mnemis__plugins__test__TestPlugin_t {
    uint offsetsAndSizes[2];
    char stringdata0[34];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_mnemis__plugins__test__TestPlugin_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_mnemis__plugins__test__TestPlugin_t qt_meta_stringdata_mnemis__plugins__test__TestPlugin = {
    {
        QT_MOC_LITERAL(0, 33)   // "mnemis::plugins::test::TestPl..."
    },
    "mnemis::plugins::test::TestPlugin"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_mnemis__plugins__test__TestPlugin[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

Q_CONSTINIT const QMetaObject mnemis::plugins::test::TestPlugin::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_mnemis__plugins__test__TestPlugin.offsetsAndSizes,
    qt_meta_data_mnemis__plugins__test__TestPlugin,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_mnemis__plugins__test__TestPlugin_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TestPlugin, std::true_type>
    >,
    nullptr
} };

void mnemis::plugins::test::TestPlugin::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *mnemis::plugins::test::TestPlugin::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *mnemis::plugins::test::TestPlugin::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_mnemis__plugins__test__TestPlugin.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "IPlugin"))
        return static_cast< IPlugin*>(this);
    if (!strcmp(_clname, "com.aleniastudios.Mnemis.IPlugin/1.0"))
        return static_cast< mnemis::plugins::IPlugin*>(this);
    return QObject::qt_metacast(_clname);
}

int mnemis::plugins::test::TestPlugin::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    return _id;
}
using namespace mnemis;
using namespace mnemis::plugins;
using namespace mnemis::plugins::test;

#ifdef QT_MOC_EXPORT_PLUGIN_V2
static constexpr unsigned char qt_pluginMetaDataV2_TestPlugin[] = {
    0xbf, 
    // "IID"
    0x02,  0x78,  0x24,  'c',  'o',  'm',  '.',  'a', 
    'l',  'e',  'n',  'i',  'a',  's',  't',  'u', 
    'd',  'i',  'o',  's',  '.',  'M',  'n',  'e', 
    'm',  'i',  's',  '.',  'I',  'P',  'l',  'u', 
    'g',  'i',  'n',  '/',  '1',  '.',  '0', 
    // "className"
    0x03,  0x6a,  'T',  'e',  's',  't',  'P',  'l', 
    'u',  'g',  'i',  'n', 
    0xff, 
};
QT_MOC_EXPORT_PLUGIN_V2(mnemis::plugins::test::TestPlugin, TestPlugin, qt_pluginMetaDataV2_TestPlugin)
#else
QT_PLUGIN_METADATA_SECTION
Q_CONSTINIT static constexpr unsigned char qt_pluginMetaData_TestPlugin[] = {
    'Q', 'T', 'M', 'E', 'T', 'A', 'D', 'A', 'T', 'A', ' ', '!',
    // metadata version, Qt version, architectural requirements
    0, QT_VERSION_MAJOR, QT_VERSION_MINOR, qPluginArchRequirements(),
    0xbf, 
    // "IID"
    0x02,  0x78,  0x24,  'c',  'o',  'm',  '.',  'a', 
    'l',  'e',  'n',  'i',  'a',  's',  't',  'u', 
    'd',  'i',  'o',  's',  '.',  'M',  'n',  'e', 
    'm',  'i',  's',  '.',  'I',  'P',  'l',  'u', 
    'g',  'i',  'n',  '/',  '1',  '.',  '0', 
    // "className"
    0x03,  0x6a,  'T',  'e',  's',  't',  'P',  'l', 
    'u',  'g',  'i',  'n', 
    0xff, 
};
QT_MOC_EXPORT_PLUGIN(mnemis::plugins::test::TestPlugin, TestPlugin)
#endif  // QT_MOC_EXPORT_PLUGIN_V2

QT_WARNING_POP
QT_END_MOC_NAMESPACE
