# automatically generated by the FlatBuffers compiler, do not modify

# namespace: Protocol

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class S_PlayerLeft(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = S_PlayerLeft()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsS_PlayerLeft(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # S_PlayerLeft
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # S_PlayerLeft
    def PlayerId(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Uint64Flags, o + self._tab.Pos)
        return 0

def S_PlayerLeftStart(builder):
    builder.StartObject(1)

def Start(builder):
    S_PlayerLeftStart(builder)

def S_PlayerLeftAddPlayerId(builder, playerId):
    builder.PrependUint64Slot(0, playerId, 0)

def AddPlayerId(builder, playerId):
    S_PlayerLeftAddPlayerId(builder, playerId)

def S_PlayerLeftEnd(builder):
    return builder.EndObject()

def End(builder):
    return S_PlayerLeftEnd(builder)
