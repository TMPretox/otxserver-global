local voc = {1, 2, 3, 4, 5, 6, 7, 8}

	arr = {
	{0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0},
	{0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
	{0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
	{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
	{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
	{1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1},
	{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
	{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
	{0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
	{0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
	{0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
	}

local area = createCombatArea(arr)

local combat = createCombatObject()
setCombatArea(combat, area)

function onTargetTile(cid, pos)
    local creatureTable = {}
    local n, i = getTileInfo({x=pos.x, y=pos.y, z=pos.z}).creatures, 1
    if n ~= 0 then
        local v = getThingfromPos({x=pos.x, y=pos.y, z=pos.z, stackpos=i}).uid
        while v ~= 0 do
            if isCreature(v) == true then
                table.insert(creatureTable, v)
                if n == #creatureTable then
                    break
                end
            end
            i = i + 1
            v = getThingfromPos({x=pos.x, y=pos.y, z=pos.z, stackpos=i}).uid
        end
    end
    if #creatureTable ~= nil and #creatureTable > 0 then
        for r = 1, #creatureTable do
            if creatureTable[r] ~= cid then
                local min = 30000
                local max = 30000
                if isPlayer(creatureTable[r]) == true and isInArray(voc, getPlayerVocation(creatureTable[r])) == true then
                    doTargetCombatHealth(cid, creatureTable[r], COMBAT_ENERGYDAMAGE, -min, -max, CONST_ME_NONE)
                elseif isMonster(creatureTable[r]) == true then
                    doTargetCombatHealth(cid, creatureTable[r], COMBAT_ENERGYDAMAGE, -min, -max, CONST_ME_NONE)
                end
            end
        end
    end
    doSendMagicEffect(pos, CONST_ME_PURPLEENERGY)
    return true
end

setCombatCallback(combat, CALLBACK_PARAM_TARGETTILE, "onTargetTile")

local function delayedCastSpell(cid, var)
    if isCreature(cid) == true then
        doCombat(cid, combat, positionToVariant(getCreaturePosition(cid)))
	doCreatureSay(cid, "Gaz'haragoth calls down: DEATH AND DOOM!", TALKTYPE_ORANGE_2)
    end
end

function onCastSpell(cid, var)
    doCreatureSay(cid, "Gaz'haragoth begins to channel DEATH AND DOOM into the area! RUN!", TALKTYPE_ORANGE_2)
    addEvent(delayedCastSpell, 5000, cid, var)
    return true
end