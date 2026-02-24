using System.Text;

namespace WCopyfindCS.Core;

/// <summary>
/// Direct C# port of Words.cpp. All static, no state.
/// WordHash produces the same values as the C++ implementation.
/// </summary>
public static class WordProcessor
{
    /// <summary>
    /// Rotate-left-7 XOR hash — identical algorithm to the original C++ WordHash().
    /// Returns 1 for empty strings (same as C++).
    /// </summary>
    public static uint WordHash(string word)
    {
        if (word.Length == 0) return 1;

        uint hash = 0;
        foreach (char c in word)
        {
            hash = ((hash << 7) | (hash >> 25)) ^ (uint)c;
        }
        return hash;
    }

    /// <summary>
    /// Remove all punctuation characters from a word (char.IsPunctuation).
    /// Port of WordRemovePunctuation().
    /// </summary>
    public static string RemovePunctuation(string word)
    {
        if (word.Length == 0) return word;
        var sb = new StringBuilder(word.Length);
        foreach (char c in word)
            if (!char.IsPunctuation(c))
                sb.Append(c);
        return sb.ToString();
    }

    /// <summary>
    /// Remove leading and trailing punctuation characters.
    /// Port of wordxouterpunct().
    /// </summary>
    public static string RemoveOuterPunctuation(string word)
    {
        int start = 0, end = word.Length - 1;
        while (start <= end && char.IsPunctuation(word[start])) start++;
        while (end >= start && char.IsPunctuation(word[end])) end--;
        return start > end ? string.Empty : word.Substring(start, end - start + 1);
    }

    /// <summary>
    /// Remove all digit characters from a word.
    /// Port of WordRemoveNumbers().
    /// </summary>
    public static string RemoveNumbers(string word)
    {
        if (word.Length == 0) return word;
        var sb = new StringBuilder(word.Length);
        foreach (char c in word)
            if (!char.IsDigit(c))
                sb.Append(c);
        return sb.ToString();
    }

    /// <summary>
    /// Convert word to lower case.
    /// Port of WordToLowerCase() — uses ToLowerInvariant for consistency.
    /// </summary>
    public static string ToLowerCase(string word) => word.ToLowerInvariant();

    /// <summary>
    /// Validate that a word is "real": starts and ends with a letter;
    /// interior may contain only letters, hyphens, or apostrophes.
    /// Port of WordCheck().
    /// </summary>
    public static bool WordCheck(string word)
    {
        if (word.Length < 1) return false;
        if (!char.IsLetter(word[0])) return false;
        if (!char.IsLetter(word[word.Length - 1])) return false;
        for (int i = 1; i < word.Length - 1; i++)
        {
            char c = word[i];
            if (char.IsLetter(c)) continue;
            if (c == '-') continue;
            if (c == '\'') continue;
            return false;
        }
        return true;
    }

    /// <summary>
    /// Apply all active word filters and return the filtered word.
    /// Returns null if the word should be skipped entirely.
    /// </summary>
    public static string? ApplyFilters(string word, ComparisonSettings s)
    {
        if (s.IgnorePunctuation)      word = RemovePunctuation(word);
        if (s.IgnoreOuterPunctuation) word = RemoveOuterPunctuation(word);
        if (s.IgnoreNumbers)          word = RemoveNumbers(word);
        if (s.IgnoreCase)             word = ToLowerCase(word);
        if (s.SkipLongWords && word.Length > s.SkipLength)  return null;
        if (s.SkipNonwords  && !WordCheck(word))             return null;
        return word;
    }
}
