require 'spec_helper'
require 'MeCab'

module MeCab
  describe Tagger do
    subject(:tagger) { described_class.new(*[options].compact) }
    let(:options) { nil }

    describe '#parse' do
      context 'When the tagger is initialized with "-O wakati"' do
        let(:options) { '-O wakati' }

        context 'When the subject method is called with "寿司とすき焼きを食べた。"' do
          let(:input) { "寿司とすき焼きを食べた。" }
          subject(:result) { tagger.parse(input) }

          it { should eq("寿司 と すき焼き を 食べ た 。 \n") }
        end
      end
    end

    describe '#parseNBest' do
      context 'When the tagger is initialized with "-l 1"' do
        let(:options) { '-l 1' }

        context 'When the subject method is called with 3 and "寿司とすき焼きを食べた。"' do
          let(:input) { "寿司とすき焼きを食べた。" }
          subject(:result) { tagger.parseNBest(3, input) }

          describe 'the surfaces for each node' do
            subject(:surfaces) { result.lines.map {|l| l.split(/[\t,]/)[0] } }

            it { should include('寿司') }
            it { should include('すき焼き') }
            it { should include('食べ') }
          end

          describe 'the normal form for each node' do
            subject(:normal_forms) { result.lines.map {|l| l.split(/[\t,]/)[7] } }

            it { should include('寿司') }
            it { should include('すき焼き') }
            it { should include('食べる') }
          end
        end
      end
    end

    describe '#parseNBestInit' do
      context 'When the tagger is initialized with "-l 1"' do
        let(:options) { '-l 1' }

        context 'When the subject method is called with 3 and "寿司とすき焼きを食べた。"' do
          let(:input) { "寿司とすき焼きを食べた。" }
          subject(:result) { tagger.parseNBestInit(input) }

          it 'does not raise error' do
            expect { subject }.to_not raise_error
          end
        end
      end
    end
  end
end
