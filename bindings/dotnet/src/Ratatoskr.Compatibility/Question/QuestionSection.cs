using System.Collections.Generic;

namespace DNS.Client;

public class QuestionSection
{
    private IList<QuestionField> questionFields = new List<QuestionField>();

    public QuestionSection(IEnumerable<Question> questions)
    {
        foreach (var question in questions)
        {
            var questionField = new QuestionField(question);
            questionFields.Add(questionField);
        }
    }

    public byte[] GetBytes()
    {
        var bytes = new List<byte>();
        foreach (var questionField in questionFields)
        {
            bytes.AddRange(questionField.GetBytes());
        }
        return bytes.ToArray();
    }
}
