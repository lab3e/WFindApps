using System.Text.Json;
using System.Text.Json.Serialization;

namespace WCopyfindCS.Properties;

/// <summary>
/// Persistent user settings. Replaces WIN32 Registry API from the original C++ code.
/// Stored in %APPDATA%\WCopyfind\settings.json as JSON.
/// Access via Settings.Default; call Settings.Default.Save() to persist.
/// </summary>
public sealed class Settings
{
    // ─── Singleton ────────────────────────────────────────────────────────────

    private static Settings? _default;

    public static Settings Default => _default ??= Load();

    // ─── Settings properties with original defaults ───────────────────────────

    public int    PhraseLength          { get; set; } = 6;
    public int    WordThreshold         { get; set; } = 100;
    public int    SkipLength            { get; set; } = 20;
    public int    MismatchTolerance     { get; set; } = 2;
    public int    MismatchPercentage    { get; set; } = 80;

    public bool   BriefReport           { get; set; } = false;
    public bool   IgnoreCase            { get; set; } = false;
    public bool   IgnoreNumbers         { get; set; } = false;
    public bool   IgnorePunctuation     { get; set; } = false;
    public bool   IgnoreOuterPunctuation{ get; set; } = false;
    public bool   SkipLongWords         { get; set; } = false;
    public bool   SkipNonwords          { get; set; } = false;
    public bool   BasicCharacters       { get; set; } = false;
    public bool   SortOnLoadOld         { get; set; } = true;
    public bool   SortOnLoadNew         { get; set; } = true;

    public string ReportFolder          { get; set; } = @"C:\WCopyfind\Report";
    public string Language              { get; set; } = "English";

    // ─── Persistence ─────────────────────────────────────────────────────────

    [JsonIgnore]
    private static string SettingsPath =>
        Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
            "WCopyfind",
            "settings.json");

    private static readonly JsonSerializerOptions JsonOpts = new()
    {
        WriteIndented = true,
    };

    private static Settings Load()
    {
        try
        {
            string path = SettingsPath;
            if (File.Exists(path))
            {
                string json = File.ReadAllText(path);
                return JsonSerializer.Deserialize<Settings>(json, JsonOpts) ?? new Settings();
            }
        }
        catch
        {
            // If settings file is corrupt or unreadable, start fresh
        }
        return new Settings();
    }

    public void Save()
    {
        try
        {
            string path = SettingsPath;
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            string json = JsonSerializer.Serialize(this, JsonOpts);
            File.WriteAllText(path, json);
        }
        catch
        {
            // Silently ignore save failures (read-only profile, locked file, etc.)
        }
    }
}
