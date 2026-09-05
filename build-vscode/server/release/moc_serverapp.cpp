/****************************************************************************
** Meta object code from reading C++ file 'serverapp.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../server/serverapp.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'serverapp.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_TlsTcpServer_t {
    QByteArrayData data[1];
    char stringdata0[13];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TlsTcpServer_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TlsTcpServer_t qt_meta_stringdata_TlsTcpServer = {
    {
QT_MOC_LITERAL(0, 0, 12) // "TlsTcpServer"

    },
    "TlsTcpServer"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TlsTcpServer[] = {

 // content:
       8,       // revision
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

void TlsTcpServer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject TlsTcpServer::staticMetaObject = { {
    QMetaObject::SuperData::link<QTcpServer::staticMetaObject>(),
    qt_meta_stringdata_TlsTcpServer.data,
    qt_meta_data_TlsTcpServer,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *TlsTcpServer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TlsTcpServer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TlsTcpServer.stringdata0))
        return static_cast<void*>(this);
    return QTcpServer::qt_metacast(_clname);
}

int TlsTcpServer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTcpServer::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_ServerApp_t {
    QByteArrayData data[5];
    char stringdata0[53];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ServerApp_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ServerApp_t qt_meta_stringdata_ServerApp = {
    {
QT_MOC_LITERAL(0, 0, 9), // "ServerApp"
QT_MOC_LITERAL(1, 10, 17), // "acceptConnections"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 10), // "readClient"
QT_MOC_LITERAL(4, 40, 12) // "removeClient"

    },
    "ServerApp\0acceptConnections\0\0readClient\0"
    "removeClient"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ServerApp[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   29,    2, 0x08 /* Private */,
       3,    0,   30,    2, 0x08 /* Private */,
       4,    0,   31,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void ServerApp::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ServerApp *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->acceptConnections(); break;
        case 1: _t->readClient(); break;
        case 2: _t->removeClient(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject ServerApp::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ServerApp.data,
    qt_meta_data_ServerApp,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ServerApp::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ServerApp::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ServerApp.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ServerApp::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
