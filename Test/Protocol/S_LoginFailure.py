# automatically generated by the FlatBuffers compiler, do not modify

# namespace: Protocol

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class S_LoginFailure(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = S_LoginFailure()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsS_LoginFailure(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # S_LoginFailure
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # S_LoginFailure
    def ErrorCode(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int32Flags, o + self._tab.Pos)
        return 0

    # S_LoginFailure
    def ErrorMessage(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # S_LoginFailure
    def CommandId(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(8))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int64Flags, o + self._tab.Pos)
        return 0

def S_LoginFailureStart(builder):
    builder.StartObject(3)

def Start(builder):
    S_LoginFailureStart(builder)

def S_LoginFailureAddErrorCode(builder, errorCode):
    builder.PrependInt32Slot(0, errorCode, 0)

def AddErrorCode(builder, errorCode):
    S_LoginFailureAddErrorCode(builder, errorCode)

def S_LoginFailureAddErrorMessage(builder, errorMessage):
    builder.PrependUOffsetTRelativeSlot(1, flatbuffers.number_types.UOffsetTFlags.py_type(errorMessage), 0)

def AddErrorMessage(builder, errorMessage):
    S_LoginFailureAddErrorMessage(builder, errorMessage)

def S_LoginFailureAddCommandId(builder, commandId):
    builder.PrependInt64Slot(2, commandId, 0)

def AddCommandId(builder, commandId):
    S_LoginFailureAddCommandId(builder, commandId)

def S_LoginFailureEnd(builder):
    return builder.EndObject()

def End(builder):
    return S_LoginFailureEnd(builder)
