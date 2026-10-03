import nltk
from nltk.tokenize import word_tokenize
from nltk.chunk import ne_chunk

# Download NLTK data
nltk.download('punkt')
nltk.download('maxent_ne_chunker')
nltk.download('words')

# User input
user_input = "Drive me home"

# Tokenization
tokens = word_tokenize(user_input)

# Named Entity Recognition (NER)
ner_tags = ne_chunk(nltk.pos_tag(tokens))

# Extract goal
goal = None
for subtree in ner_tags:
    if isinstance(subtree, nltk.Tree) and subtree.label() == 'GPE':
        entity = " ".join([token for token, _ in subtree.leaves()])
        goal = f"drive to {entity}"
        break

if goal:
    print("Extracted Goal:", goal)
else:
    print("No goal extracted.")