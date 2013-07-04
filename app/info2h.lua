dofile '../info.lua'

io.open('info.h', 'w')
	:write(([[
constexpr int Revision = %d;
]]):format(Info.Version))
	:close()
