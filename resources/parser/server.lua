local parsingEnabled = {}

RegisterCommand('toggleparser', function(source)
        if source == 0 then return end
        parsingEnabled[source] = not parsingEnabled[source]
        if not parsingEnabled[source] then
                TriggerClientEvent('parser:setState', source, false)
                print(('[parser] Player %s disabled log parsing.'):format(source))
                return
        end
        TriggerClientEvent('parser:setState', source, true)
        print(('[parser] Player %s enabled log parsing.'):format(source))
end, false)

AddEventHandler('playerDropped', function()
        parsingEnabled[source] = nil
end)
