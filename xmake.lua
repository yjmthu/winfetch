set_languages("cxxlatest")

add_rules("mode.release", "mode.debug")

if is_plat("windows") then
  add_defines("UNICODE", "_UNICODE")
  add_cxflags("/utf-8")
end

add_includedirs("include")

target("winfetch")
  set_kind("binary")
  add_files("src/winfetch.cpp")
  add_syslinks("Advapi32", "Version", "DXGI", "UxTheme", "User32")
  after_build(function (target)
    import("core.project.task")
    task.run("project", {kind = "compile_commands", outputdir = "build"})
  end)
target_end()

