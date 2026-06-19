-- Shared logging module. Require'd by other scripts to test inter-script calls.
local Logger = {}

function Logger.info(msg)
    print("[LUA INFO] " .. tostring(msg))
end

function Logger.warn(msg)
    print("[LUA WARN] " .. tostring(msg))
end

function Logger.error(msg)
    print("[LUA ERROR] " .. tostring(msg))
end

return Logger
