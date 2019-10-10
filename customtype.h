#ifndef CUSTOMTYPE_H
#define CUSTOMTYPE_H

#include <QDBusMetaType>
#include <QDBusVariant>

struct StructUdu
{
    quint32 time;
    qreal value;
    quint32 state;
};

QDBusArgument &operator<<(QDBusArgument &argument, const StructUdu &structudp);
const QDBusArgument &operator>>(const QDBusArgument &argument, StructUdu &structudp);
QDBusArgument &operator<<(QDBusArgument &argument, const QList<StructUdu> &myarray);
const QDBusArgument &operator>>(const QDBusArgument &argument, QList<StructUdu> &myarray);

void registerCustomType();


Q_DECLARE_METATYPE(QList<StructUdu>)
Q_DECLARE_METATYPE(StructUdu)


/**
 * UpDeviceKind:
 *
 * The device type.
 **/
typedef enum {
    UP_DEVICE_KIND_UNKNOWN,
    UP_DEVICE_KIND_LINE_POWER,
    UP_DEVICE_KIND_BATTERY,
    UP_DEVICE_KIND_UPS,
    UP_DEVICE_KIND_MONITOR,
    UP_DEVICE_KIND_MOUSE,
    UP_DEVICE_KIND_KEYBOARD,
    UP_DEVICE_KIND_PDA,
    UP_DEVICE_KIND_PHONE,
    UP_DEVICE_KIND_MEDIA_PLAYER,
    UP_DEVICE_KIND_TABLET,
    UP_DEVICE_KIND_COMPUTER,
    UP_DEVICE_KIND_LAST
} UpDeviceKind;

/**
 * UpDeviceState:
 *
 * The device state.
 **/
typedef enum {
    UP_DEVICE_STATE_UNKNOWN,
    UP_DEVICE_STATE_CHARGING,
    UP_DEVICE_STATE_DISCHARGING,
    UP_DEVICE_STATE_EMPTY,
    UP_DEVICE_STATE_FULLY_CHARGED,
    UP_DEVICE_STATE_PENDING_CHARGE,
    UP_DEVICE_STATE_PENDING_DISCHARGE,
    UP_DEVICE_STATE_LAST
} UpDeviceState;

/**
 * UpDeviceTechnology:
 *
 * The device technology.
 **/
typedef enum {
    UP_DEVICE_TECHNOLOGY_UNKNOWN,
    UP_DEVICE_TECHNOLOGY_LITHIUM_ION,
    UP_DEVICE_TECHNOLOGY_LITHIUM_POLYMER,
    UP_DEVICE_TECHNOLOGY_LITHIUM_IRON_PHOSPHATE,
    UP_DEVICE_TECHNOLOGY_LEAD_ACID,
    UP_DEVICE_TECHNOLOGY_NICKEL_CADMIUM,
    UP_DEVICE_TECHNOLOGY_NICKEL_METAL_HYDRIDE,
    UP_DEVICE_TECHNOLOGY_LAST
} UpDeviceTechnology;

/**
 * UpDeviceLevel:
 *
 * The warning level of a battery.
 **/
typedef enum {
    UP_DEVICE_LEVEL_UNKNOWN,
    UP_DEVICE_LEVEL_NONE,
    UP_DEVICE_LEVEL_DISCHARGING,
    UP_DEVICE_LEVEL_LOW,
    UP_DEVICE_LEVEL_CRITICAL,
    UP_DEVICE_LEVEL_ACTION,
    UP_DEVICE_LEVEL_LAST
} UpDeviceLevel;

const char	*up_device_kind_to_string		(UpDeviceKind		 type_enum);
//const gchar	*up_device_state_to_string		(UpDeviceState		 state_enum);
//const gchar	*up_device_technology_to_string		(UpDeviceTechnology	 technology_enum);
//const gchar	*up_device_level_to_string		(UpDeviceLevel		 level_enum);
//UpDeviceKind	 up_device_kind_from_string		(const gchar		*type);
//UpDeviceState	 up_device_state_from_string		(const gchar		*state);
//UpDeviceTechnology up_device_technology_from_string	(const gchar		*technology);
//UpDeviceLevel	 up_device_level_from_string		(const gchar		*level);
const char *
gpm_device_kind_to_localised_text (UpDeviceKind kind, uint number);




#endif // CUSTOMTYPE_H
