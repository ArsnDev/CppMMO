# automatically generated by the FlatBuffers compiler, do not modify

# namespace: Protocol

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class C_EnterZone(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = C_EnterZone()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsC_EnterZone(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # C_EnterZone
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # C_EnterZone
    def ZoneId(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int32Flags, o + self._tab.Pos)
        return 0

    # C_EnterZone
    def CommandId(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int64Flags, o + self._tab.Pos)
        return 0

def C_EnterZoneStart(builder):
    builder.StartObject(2)

def Start(builder):
    C_EnterZoneStart(builder)

def C_EnterZoneAddZoneId(builder, zoneId):
    builder.PrependInt32Slot(0, zoneId, 0)

def AddZoneId(builder, zoneId):
    C_EnterZoneAddZoneId(builder, zoneId)

def C_EnterZoneAddCommandId(builder, commandId):
    builder.PrependInt64Slot(1, commandId, 0)

def AddCommandId(builder, commandId):
    C_EnterZoneAddCommandId(builder, commandId)

def C_EnterZoneEnd(builder):
    return builder.EndObject()

def End(builder):
    return C_EnterZoneEnd(builder)
