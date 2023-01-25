
#ifndef TRANSFORMER_HPP
#define TRANSFORMER_HPP

struct TransformSpec {
  unsigned long long a;
  unsigned long long b;
  unsigned long long m;
  int iterations;
};

class Transformer {
public:
  Transformer() {};
  ~Transformer() {};

  // the producer's work
  unsigned long long producer_transform(char opcode, unsigned long long val);

  // the consumer's work
  unsigned long long consumer_transform(char opcode, unsigned long long val);

private:
  unsigned long long transform(TransformSpec* spec, unsigned long long val);
};

#endif // TRANSFORMER_HPP
