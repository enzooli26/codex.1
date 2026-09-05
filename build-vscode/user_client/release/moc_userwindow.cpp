/****************************************************************************
** Meta object code from reading C++ file 'userwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../user_client/userwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'userwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_UserWindow_t {
    QByteArrayData data[12];
    char stringdata0[132];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_UserWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_UserWindow_t qt_meta_stringdata_UserWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "UserWindow"
QT_MOC_LITERAL(1, 11, 13), // "connectServer"
QT_MOC_LITERAL(2, 25, 0), // ""
QT_MOC_LITERAL(3, 26, 12), // "readMessages"
QT_MOC_LITERAL(4, 39, 5), // "login"
QT_MOC_LITERAL(5, 45, 12), // "registerUser"
QT_MOC_LITERAL(6, 58, 8), // "recharge"
QT_MOC_LITERAL(7, 67, 15), // "refreshStations"
QT_MOC_LITERAL(8, 83, 7), // "reserve"
QT_MOC_LITERAL(9, 91, 17), // "cancelReservation"
QT_MOC_LITERAL(10, 109, 11), // "startCharge"
QT_MOC_LITERAL(11, 121, 10) // "stopCharge"

    },
    "UserWindow\0connectServer\0\0readMessages\0"
    "login\0registerUser\0recharge\0refreshStations\0"
    "reserve\0cancelReservation\0startCharge\0"
    "stopCharge"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_UserWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   64,    2, 0x08 /* Private */,
       3,    0,   65,    2, 0x08 /* Private */,
       4,    0,   66,    2, 0x08 /* Private */,
       5,    0,   67,    2, 0x08 /* Private */,
       6,    0,   68,    2, 0x08 /* Private */,
       7,    0,   69,    2, 0x08 /* Private */,
       8,    0,   70,    2, 0x08 /* Private */,
       9,    0,   71,    2, 0x08 /* Private */,
      10,    0,   72,    2, 0x08 /* Private */,
      11,    0,   73,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void UserWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<UserWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->connectServer(); break;
        case 1: _t->readMessages(); break;
        case 2: _t->login(); break;
        case 3: _t->registerUser(); break;
        case 4: _t->recharge(); break;
        case 5: _t->refreshStations(); break;
        case 6: _t->reserve(); break;
        case 7: _t->cancelReservation(); break;
        case 8: _t->startCharge(); break;
        case 9: _t->stopCharge(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject UserWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_UserWindow.data,
    qt_meta_data_UserWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *UserWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *UserWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_UserWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int UserWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 10;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
