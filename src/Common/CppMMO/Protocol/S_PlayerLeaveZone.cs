// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

namespace CppMMO.Protocol
{

using global::System;
using global::System.Collections.Generic;
using global::Google.FlatBuffers;

public struct S_PlayerLeaveZone : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static void ValidateVersion() { FlatBufferConstants.FLATBUFFERS_25_2_10(); }
  public static S_PlayerLeaveZone GetRootAsS_PlayerLeaveZone(ByteBuffer _bb) { return GetRootAsS_PlayerLeaveZone(_bb, new S_PlayerLeaveZone()); }
  public static S_PlayerLeaveZone GetRootAsS_PlayerLeaveZone(ByteBuffer _bb, S_PlayerLeaveZone obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p = new Table(_i, _bb); }
  public S_PlayerLeaveZone __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public long PlayerId { get { int o = __p.__offset(4); return o != 0 ? __p.bb.GetLong(o + __p.bb_pos) : (long)0; } }

  public static Offset<CppMMO.Protocol.S_PlayerLeaveZone> CreateS_PlayerLeaveZone(FlatBufferBuilder builder,
      long player_id = 0) {
    builder.StartTable(1);
    S_PlayerLeaveZone.AddPlayerId(builder, player_id);
    return S_PlayerLeaveZone.EndS_PlayerLeaveZone(builder);
  }

  public static void StartS_PlayerLeaveZone(FlatBufferBuilder builder) { builder.StartTable(1); }
  public static void AddPlayerId(FlatBufferBuilder builder, long playerId) { builder.AddLong(0, playerId, 0); }
  public static Offset<CppMMO.Protocol.S_PlayerLeaveZone> EndS_PlayerLeaveZone(FlatBufferBuilder builder) {
    int o = builder.EndTable();
    return new Offset<CppMMO.Protocol.S_PlayerLeaveZone>(o);
  }
}


static public class S_PlayerLeaveZoneVerify
{
  static public bool Verify(Google.FlatBuffers.Verifier verifier, uint tablePos)
  {
    return verifier.VerifyTableStart(tablePos)
      && verifier.VerifyField(tablePos, 4 /*PlayerId*/, 8 /*long*/, 8, false)
      && verifier.VerifyTableEnd(tablePos);
  }
}

}
