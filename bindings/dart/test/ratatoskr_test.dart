import 'package:ratatoskr/ratatoskr.dart';
import 'package:test/test.dart';

void main() {
  test('loads native ABI 1', () => expect(Ratatoskr().abiVersion, 1));
}
