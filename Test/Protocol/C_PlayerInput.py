# automatically generated by the FlatBuffers compiler, do not modify

# namespace: Protocol

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class C_PlayerInput(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = C_PlayerInput()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsC_PlayerInput(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # C_PlayerInput
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # C_PlayerInput
    def TickNumber(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Uint64Flags, o + self._tab.Pos)
        return 0

    # C_PlayerInput
    def ClientTime(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Uint64Flags, o + self._tab.Pos)
        return 0

    # C_PlayerInput
    def InputFlags(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(8))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Uint8Flags, o + self._tab.Pos)
        return 0

    # C_PlayerInput
    def MousePosition(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(10))
        if o != 0:
            x = self._tab.Indirect(o + self._tab.Pos)
            from .Vec3 import Vec3
            obj = Vec3()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # C_PlayerInput
    def SequenceNumber(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(12))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Uint32Flags, o + self._tab.Pos)
        return 0

    # C_PlayerInput
    def CommandId(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(14))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int64Flags, o + self._tab.Pos)
        return 0

def C_PlayerInputStart(builder):
    builder.StartObject(6)

def Start(builder):
    C_PlayerInputStart(builder)

def C_PlayerInputAddTickNumber(builder, tickNumber):
    builder.PrependUint64Slot(0, tickNumber, 0)

def AddTickNumber(builder, tickNumber):
    C_PlayerInputAddTickNumber(builder, tickNumber)

def C_PlayerInputAddClientTime(builder, clientTime):
    builder.PrependUint64Slot(1, clientTime, 0)

def AddClientTime(builder, clientTime):
    C_PlayerInputAddClientTime(builder, clientTime)

def C_PlayerInputAddInputFlags(builder, inputFlags):
    builder.PrependUint8Slot(2, inputFlags, 0)

def AddInputFlags(builder, inputFlags):
    C_PlayerInputAddInputFlags(builder, inputFlags)

def C_PlayerInputAddMousePosition(builder, mousePosition):
    builder.PrependUOffsetTRelativeSlot(3, flatbuffers.number_types.UOffsetTFlags.py_type(mousePosition), 0)

def AddMousePosition(builder, mousePosition):
    C_PlayerInputAddMousePosition(builder, mousePosition)

def C_PlayerInputAddSequenceNumber(builder, sequenceNumber):
    builder.PrependUint32Slot(4, sequenceNumber, 0)

def AddSequenceNumber(builder, sequenceNumber):
    C_PlayerInputAddSequenceNumber(builder, sequenceNumber)

def C_PlayerInputAddCommandId(builder, commandId):
    builder.PrependInt64Slot(5, commandId, 0)

def AddCommandId(builder, commandId):
    C_PlayerInputAddCommandId(builder, commandId)

def C_PlayerInputEnd(builder):
    return builder.EndObject()

def End(builder):
    return C_PlayerInputEnd(builder)
