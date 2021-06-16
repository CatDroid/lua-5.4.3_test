local mri = require "MemoryReferenceInfo"
mri.m_cConfig.m_bAllMemoryRefFileAddTime = false    -- snapshot文件名字是否增加时间戳
mri.m_cConfig.m_bComparedMemoryRefFileAddTime = false -- compare文件名字是否增加时间戳

-- filename:String
-- index:Integer
function Snapshot(filename, index) -- 通过index来区分不同的snapchat
  mri.m_cMethods.DumpMemorySnapshot("./", filename..tostring(index) , -1);
  return true ;
end

function Compare(snapshotFileName, compareFileName, beforeIndex, afterIndex, compareIndex)

  print("[LUA LOG] Compare "..tostring(snapshotFileName)..","..tostring(compareIndex))

  mri.m_cMethods.DumpMemorySnapshotComparedFile(
    "./",
    compareFileName..tostring(compareIndex),
    -1,
    "./LuaMemRefInfo-All-["..snapshotFileName..tostring(beforeIndex).."].txt",
    "./LuaMemRefInfo-All-["..snapshotFileName..tostring(afterIndex).."].txt")
  return true ;
end

print("[LUA LOG] run DumpLua.lua")


