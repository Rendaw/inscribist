#!/usr/bin/lua
Success = os.execute('selfdiscovery configure-controller.lua Debug ' .. table.concat(arg, ' ', 1))
if not Success then return 1 else return 0 end
