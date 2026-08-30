namespace DNS.Client;

public class Question
{
    public string Domain { get; set; } = string.Empty;
    public QuestionType Type { get; set; }
    public QuestionClass Class { get; set; }
}
