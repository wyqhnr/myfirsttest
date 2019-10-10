#include "customtype.h"
#include <libintl.h>

void registerCustomType()
{
//    qRegisterMetaType<StructUdu>("StructUdu");
//    qDBusRegisterMetaType<QList<QDBusVariant>>();
//    qRegisterMetaType<QList<QDBusVariant>>("QList<QDBusVariant>");
    qDBusRegisterMetaType<StructUdu>();
    qDBusRegisterMetaType<QList<StructUdu>>();

}


QDBusArgument &operator<<(QDBusArgument &argument, const StructUdu &structudp)
{
    argument.beginStructure();
    argument<<structudp.time<<structudp.value<<structudp.state;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, StructUdu &structudp)
{
    argument.beginStructure();
    argument>>structudp.time>>structudp.value>>structudp.state;
    argument.endStructure();
    return argument;
}


QDBusArgument &operator<<(QDBusArgument &argument, const QList<StructUdu> &myarray)
{
    argument.beginArray(qMetaTypeId<StructUdu>());
    for(int i= 0; i<myarray.length(); i++)
        argument << myarray.at(i);
    argument.endArray();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QList<StructUdu> &myarray)
{
    argument.beginArray();
    myarray.clear();
    while(!argument.atEnd())
    {
        StructUdu element;
        argument>>element;
        myarray.append(element);
    }
    argument.endArray();
    return argument;
}

//////up-types.c
const char *
up_device_kind_to_string (UpDeviceKind type_enum)
{
    switch (type_enum) {
    case UP_DEVICE_KIND_LINE_POWER:
        return "line-power";
    case UP_DEVICE_KIND_BATTERY:
        return "battery";
    case UP_DEVICE_KIND_UPS:
        return "ups";
    case UP_DEVICE_KIND_MONITOR:
        return "monitor";
    case UP_DEVICE_KIND_MOUSE:
        return "mouse";
    case UP_DEVICE_KIND_KEYBOARD:
        return "keyboard";
    case UP_DEVICE_KIND_PDA:
        return "pda";
    case UP_DEVICE_KIND_PHONE:
        return "phone";
    case UP_DEVICE_KIND_MEDIA_PLAYER:
        return "media-player";
    case UP_DEVICE_KIND_TABLET:
        return "tablet";
    case UP_DEVICE_KIND_COMPUTER:
        return "computer";
    default:
        return "unknown";
    }
}

/**
 * gpm_device_kind_to_localised_text:
 **/
const char *
gpm_device_kind_to_localised_text (UpDeviceKind kind, uint number)
{
    const char *text = NULL;
    switch (kind) {
    case UP_DEVICE_KIND_LINE_POWER:
        /* TRANSLATORS: system power cord */
        text = ngettext ("AC adapter", "AC adapters", number);
        break;
    case UP_DEVICE_KIND_BATTERY:
        /* TRANSLATORS: laptop primary battery */
        text = ngettext ("Laptop battery", "Laptop batteries", number);
        break;
    case UP_DEVICE_KIND_UPS:
        /* TRANSLATORS: battery-backed AC power source */
        text = ngettext ("UPS", "UPSs", number);
        break;
    case UP_DEVICE_KIND_MONITOR:
        /* TRANSLATORS: a monitor is a device to measure voltage and current */
        text = ngettext ("Monitor", "Monitors", number);
        break;
    case UP_DEVICE_KIND_MOUSE:
        /* TRANSLATORS: wireless mice with internal batteries */
        text = ngettext ("Mouse", "Mice", number);
        break;
    case UP_DEVICE_KIND_KEYBOARD:
        /* TRANSLATORS: wireless keyboard with internal battery */
        text = ngettext ("Keyboard", "Keyboards", number);
        break;
    case UP_DEVICE_KIND_PDA:
        /* TRANSLATORS: portable device */
        text = ngettext ("PDA", "PDAs", number);
        break;
    case UP_DEVICE_KIND_PHONE:
        /* TRANSLATORS: cell phone (mobile...) */
        text = ngettext ("Cell phone", "Cell phones", number);
        break;
    case UP_DEVICE_KIND_MEDIA_PLAYER:
        /* TRANSLATORS: media player, mp3 etc */
        text = ngettext ("Media player", "Media players", number);
        break;
    case UP_DEVICE_KIND_TABLET:
        /* TRANSLATORS: tablet device */
        text = ngettext ("Tablet", "Tablets", number);
        break;
    case UP_DEVICE_KIND_COMPUTER:
        /* TRANSLATORS: tablet device */
        text = ngettext ("Computer", "Computers", number);
        break;
    default:
        printf ("enum unrecognised: %i", kind);
        text = up_device_kind_to_string (kind);
    }
    return text;
}
