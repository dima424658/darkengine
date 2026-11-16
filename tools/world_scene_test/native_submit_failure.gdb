set pagination off
set confirm off
set debuginfod enabled off
set startup-with-shell off
break RenderBackendRenderWorld
commands
silent
if $fixture_sword
call ((char*(*)(const char*)) &CommandExecute)("inv_select sword")
printf "Fixture sword selection requested\n"
end
set *(int*)&dark_fail_submit=1
set *(unsigned*)&dark_skip_modern_submits=$fixture_skip
disable 1
continue
end
break GamepadTerm
commands
silent
printf "Fixture submit failure failed=%u recovered=%u total_modern=%u\n", *(unsigned*)&dark_failed_submits, *(unsigned*)&dark_recovered_modern_submits, *(unsigned*)&dark_successful_modern_submits
continue
end
run
